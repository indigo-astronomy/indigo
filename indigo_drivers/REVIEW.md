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
| DRV-025 | Medium | `ccd_svb/indigo_ccd_svb.c:194` | SVBONY normal connect calls `SVBOpenCamera()` with only the device `usb_mutex`, but hot-plug serializes SDK enumeration and temporary open/close with `indigo_device_enumeration_mutex`. | Closed (fixed) |
| DRV-026 | Medium | `ccd_touptek/indigo_ccd_touptek.c:944` | ToupTek/OEM connect paths open devices with the vendor SDK without the hot-plug `mutex`, while the hot-plug refresh path enumerates devices and mutates shared presence state under that mutex. | Closed (fixed) |
| DRV-027 | Medium | `ccd_dsi/indigo_ccd_dsi.c:271` | Meade DSI connect opened the camera outside the hot-plug enumeration mutex, while plug/unplug scans and non-macOS temporary probe opens were serialized with that mutex. | Closed (fixed) |
| DRV-028 | Medium | `ccd_qsi/indigo_ccd_qsi.cpp:374` | QSI hot-plug uses the global `QSICamera cam` under `indigo_device_enumeration_mutex`, but connect used the same SDK object without that mutex for connect-time SDK calls. | Closed (fixed) |
| DRV-029 | High | `guider_asi/indigo_guider_asi.c:389` | `process_plug_event()` locks `indigo_device_enumeration_mutex` and never unlocks it on the successful attach path, so the first successful ASI USB-ST4 plug event can permanently block later plug/unplug enumeration. | Closed (fixed) |
| DRV-030 | Medium | `ccd_touptek/indigo_ccd_touptek.c:1123` | The disconnect path in `ccd_connect_callback()` calls `SDK_CALL(Stop)(PRIVATE_DATA->handle)` unconditionally before checking whether `handle` is non-NULL. If a prior connect attempt left `handle == NULL` (because `SDK_CALL(Open)` failed), an explicit disconnect request from a client will pass NULL to the vendor SDK, likely crashing the process. Guard the Stop call with `if (PRIVATE_DATA->handle)` or move it inside the existing `if (PRIVATE_DATA->handle)` close block. | Closed (fixed) |
| DRV-031 | High | `../indigo_tools/indigo_generator.c:1450`, `mount_ioptron/indigo_mount_ioptron.c:1470`, `mount_ioptron/indigo_mount_ioptron.c:2046` | The generated multi-device connection-handler template incremented the shared connection count before driver-specific `on_connect` initialization, but on init failure emitted only `PRIVATE_DATA->count--` and no close when that failed attempt had opened the shared handle. `mount_ioptron` exposed this generator bug in both mount and guider handlers. | Closed (fixed) |
| DRV-032 | Medium | `mount_ioptron/indigo_mount_ioptron.c:897` | V2.5/V3 guide-rate commands format RA and DEC as fixed two-digit fields (`:RG%02d%02d#`), while the inherited mount guide-rate property still accepts values up to 100. Sending 100 produces a six-digit payload (`RG100100`) that does not match the parsed two-two digit protocol shape. | Closed (fixed) |
| DRV-033 | Medium | `mount_ioptron/indigo_mount_ioptron.c:800` | `ioptron_set_tracking_rate()` validates the `:RT*#` and custom-rate replies, but then treats any readable `:ST1#` response as success instead of requiring the success ack byte. A rejected tracking-enable command can still leave `MOUNT_TRACK_RATE` reported OK. | Closed (fixed) |
| DRV-034 | High | `mount_lx200/indigo_mount_lx200.c:2333`, `mount_lx200/indigo_mount_lx200.c:3032`, `mount_lx200/indigo_mount_lx200.c:3161`, `mount_lx200/indigo_mount_lx200.c:3334` | LX200 connect handlers increment the shared `device_count` before autodetection, call `meade_close()` on autodetect failure, and then the common failure path decrements the already reset counter. A single failed detect can underflow the shared count and prevent later reconnect attempts from reopening the serial handle. | Closed (fixed) |
| DRV-035 | Medium | `mount_lx200/indigo_mount_lx200.c:3173`, `mount_lx200/indigo_mount_lx200.c:3391` | If the focuser or AUX logical device is the first LX200 device to open the shared serial connection and autodetection succeeds with an unsupported mount type, the handler decrements `device_count` and reports `CONNECTION` alert without closing the handle opened by that same attempt. The serial/TCP endpoint can stay occupied while the shared count is zero. | Closed (fixed) |
| DRV-036 | High | `mount_synscan/indigo_mount_synscan_driver.c:75` | `synscan_open()` parses `synscan://host:port` by copying `colon - host` bytes into `char host_name[INDIGO_NAME_SIZE]` with no length check and no guaranteed terminator. A long user-supplied `DEVICE_PORT` host segment can overflow the stack before `indigo_open_udp()` is called. | Closed (fixed) |
| DRV-037 | Medium | `mount_synscan/indigo_mount_synscan_guider.c:179`, `mount_synscan/indigo_mount_synscan_guider.c:237`, `mount_synscan/indigo_mount_synscan_guider.c:258` | The SynScan guider starts two long-lived pulse worker callbacks that wait on condition variables, but disconnect sets `guiding_thread_exit = false` and never signals either condition. Disconnecting the guider without detaching leaves the workers blocked and a later reconnect can start another pair of workers against the same shared state. | Closed (fixed) |
| DRV-038 | High | `mount_asi/indigo_mount_asi.c:180`, `mount_asi/indigo_mount_asi.c:876`, `mount_asi/indigo_mount_asi.c:889`, `mount_asi/indigo_mount_asi.c:1422` | The ZWO AM connect paths shared the DRV-034/DRV-035 defects, but unrecoverably: `asi_close()` reset `device_count` only when a handle was open, the handshake failure branch closed the shared handle while a sibling device could still hold it, and both failure paths decremented the count unguarded. A failed handshake, or a link lost mid-session, drove `device_count` negative, after which `device_count++ == 0` never held again and `asi_open()` was never called for the life of the loaded driver. | Closed (fixed) |
| DRV-039 | High | `focuser_asi/indigo_focuser_asi.driver:657`, `focuser_asi/indigo_focuser_asi.driver:680`, `focuser_asi/indigo_focuser_asi.c:585`, `focuser_asi/indigo_focuser_asi.c:607` | POSITION and STEPS on_change blocks merely queue another callback without a `_finalizer` marker, so generated handlers set and publish OK before issuing EAFMove. Neither callback restores BUSY, and the old dispatch logic that marked both motion properties BUSY and rejected overlapping absolute moves is gone. Clients can treat a still-moving focuser as finished; automatic compensation can also pass its position-state guard during a relative move. Move command setup into on_change, preserve both motion states and validation, and let the actual motion finalizer publish completion, as wheel_asi does. | Closed (fixed in working tree, 2026-09-07) |
| DRV-040 | Medium | `focuser_asi/indigo_focuser_asi.driver:728`, `focuser_asi/indigo_focuser_asi.c:650`, `focuser_asi/indigo_focuser_asi.c:786` | The inherited CONFIG block intercepts every configuration request and returns before indigo_focuser_change_property. It only saves EAF_BEEP: LOAD/REMOVE become no-ops, standard focuser properties are not saved, and base-driver save-file finalization and switch reset are skipped despite an OK update. Use persistent metadata for EAF_BEEP and base CONFIG handling, or preserve synchronous pass-through to the base handler. | Closed (fixed in working tree, 2026-09-07) |
| DRV-041 | High | `focuser_asi/indigo_focuser_asi.driver:634` | After EAFStop succeeds, abort cancels polling, clears moving and publishes OK without EAFIsMoving confirmation. The SDK distinguishes EAFStop from EAFStopAndWait, and documents that hand-controller movement cannot be stopped by EAFStop. Keep completion polling until the motor is confirmed stopped; do not accept new motion based only on stop-command success. | Closed (fixed and verified, 2026-09-07) |
| DRV-042 | Medium | `focuser_asi/indigo_focuser_asi.driver:200` | A failed compensation EAFMove sets position ALERT and returns without polling/recovery. Every later compensation call requires position OK, so one transient error disables further automatic attempts until another action clears the state. Distinguish recoverable command failure from an active move and define a recovery path. | Closed (fixed and verified, 2026-09-07) |
| DRV-043 | Medium | `focuser_asi/indigo_focuser_asi.driver:555` | FOCUSER_LIMITS updates the hardware maximum and its own value, but POSITION/STEPS maximums remain the attach-time info.MaxStep. Relative/automatic clamping and absolute validation use those stale maximums. A lowered limit still permits commands the SDK rejects; a raised limit can remain inaccessible. Refresh movement ranges from the effective hardware limit on connect and after setting it. | Closed (fixed and verified, 2026-09-07) |
| DRV-044 | Medium | `focuser_asi/indigo_focuser_asi.driver:293` | Connect-time required reads only log failures and never clear connection_result. The generated connection handler therefore reports success with stale/zero position or maximum data after EAFOpen succeeded but initialization failed. EAFStepRange failure explicitly overwrites the limit range with zero. Fail or explicitly degrade initialization rather than publishing valid-looking data. | Closed (fixed and verified, 2026-09-07) |
| DRV-045 | Medium | `focuser_asi/indigo_focuser_asi.driver:242` | temp starts at -273, so SDK errors that do not write the output, including EAF_ERROR_REMOVED, are reported as an absent sensor/IDLE instead of ALERT. The same failure resets prev_temp even in AUTO, discarding uncompensated temperature change when readings recover. Interpret SDK status before its output and preserve the last valid compensation baseline on transient errors. | Closed (fixed and verified, 2026-09-07) |
| DRV-046 | Medium | `focuser_asi/indigo_focuser_asi.driver:564; focuser_asi/indigo_focuser_asi.driver:580` | After successful SetMaxStep/SetBacklash, a failed GetMaxStep/GetBacklash only logs an error. The generated handler leaves the property OK and publishes the requested cached value as if readback succeeded. Mark readback failure ALERT and preserve a distinction between requested and confirmed settings. | Closed (fixed and verified, 2026-09-07) |
| DRV-047 | Medium | `focuser_asi/indigo_focuser_asi.driver:72; focuser_asi/indigo_focuser_asi.c:839` | SDK ID reservation occurs before generated attach finds a free slot or succeeds. Failure frees private_data without clearing connected_ids, and unplug cannot clear it because no devices entry exists. The rejected focuser stays skipped on replug. The generated capacity is five rather than the ten listed in REFACTOR.md, making this reachable with a sixth EAF. Fixed by detecting IDs among attached devices; the generator default capacity of five is retained as requested. | Closed (fixed and verified, 2026-09-07) |
| DRV-048 | Medium | `focuser_asi/indigo_focuser_asi.driver` (`focuser_move_finalizer`) | Supplied Claude finding: polling kept rescheduling every 0.5 seconds on SDK errors while the cached moving flag blocked later requests. Error completion now stops the loop and clears the polling flag; SDK preflight prevents treating that flag as proof the motor stopped. | Closed (fixed and verified, 2026-09-07) |
| DRV-049 | Medium | `focuser_asi/indigo_focuser_asi.driver` (`FOCUSER_ABORT_MOTION.on_change`) | Supplied Claude finding: EAFStop failure returned before resetting the momentary abort switch. The switch now resets before the SDK operation on both paths. | Closed (fixed and verified, 2026-09-07) |
| DRV-050 | Medium | `focuser_asi/indigo_focuser_asi.driver` (`FOCUSER_COMPENSATION`) | Public-bus regression testing found that coefficient/threshold changes had no driver branch and the base focuser handler did not accept them. Added the generator's empty on_change block so requested compensation settings are copied and acknowledged. | Closed (fixed and verified, 2026-09-07) |
| DRV-051 | Medium | `wheel_asi/indigo_wheel_asi.driver:76`; `wheel_asi/indigo_wheel_asi.c:455` | Focused follow-up confirms the same stale SDK ID reservation as DRV-047: connected_ids is set in sdk.plug before generated attach finds a slot or succeeds. Failed attach/capacity exhaustion frees private_data without releasing the reservation; sdk.unplug cannot clear it because no attached device entry exists. Replug continues skipping the ID until driver reinitialization. Use the actually attached devices to detect duplicate SDK IDs, as in the corrected focuser_asi. | Closed (fixed and verified, 2026-09-07) |
| DRV-052 | Medium | `rotator_asi/indigo_rotator_asi.c:732` | Potential macOS HIDAPI hot-plug enumeration failure: bundled libCAA.a defines HIDAPI with a persistent IOHIDManager bound to the initializing thread run loop. Hot-plug SDK enumeration runs in fresh timer callback threads, so later enumeration can pump a different loop and reuse stale manager state. Binary structure and thread dispatch verified statically; reproduce with hardware before claiming observed failure. | Open (hardware verification pending) |
| DRV-053 | Medium | `guider_asi/indigo_guider_asi.c:473` | Potential macOS HIDAPI hot-plug enumeration failure: bundled libUSB2ST4Conv.a defines HIDAPI with a persistent IOHIDManager bound to the initializing thread run loop. Hot-plug SDK enumeration runs in fresh timer callback threads, so later enumeration can pump a different loop and reuse stale manager state. Binary structure and thread dispatch verified statically; reproduce with hardware before claiming observed failure. | Open (hardware verification pending) |
| DRV-054 | Medium | `wheel_playerone/indigo_wheel_playerone.c:589` | Potential macOS HIDAPI hot-plug enumeration failure: bundled libPlayerOnePW.dylib defines HIDAPI with a persistent IOHIDManager bound to the initializing thread run loop. Hot-plug SDK enumeration runs in fresh timer callback threads, so later enumeration can pump a different loop and reuse stale manager state. Binary structure and thread dispatch verified statically; reproduce with hardware before claiming observed failure. | Open (hardware verification pending) |
| DRV-055 | High | `wheel_asi/indigo_wheel_asi.driver:195` | Same initialization problem as DRV-044: EFWGetProperty status is ignored and uninitialized info.slotNum is copied to property counts/max; EFWGetPosition status is also ignored and connection_result remains true. Invalid counts can exceed allocated item arrays. Failed position read reporting successful connection reproduced; uninitialized counts checked statically. Honor required SDK read results before consuming outputs and close/release the SDK handle when initialization fails. | Closed (fixed and verified, 2026-09-07) |
| DRV-056 | Medium | `wheel_asi/indigo_wheel_asi.driver:160` | Related to DRV-048/046: move finalizer ignores EFWGetPosition failure, increments cached current_slot, and compares this fabricated position to the target. Repeated failed reads can falsely finish with OK or continue polling indefinitely if the target is already below the stale value. Reproduced fabricated slot 5/OK with all reads failing. Use a local SDK result, preserve confirmed position on failure, and terminate/report ALERT. | Closed (fixed and verified, 2026-09-07) |
| DRV-057 | Medium | `wheel_asi/indigo_wheel_asi.driver:176` | Related to DRV-041/045/046: calibration finalizer initializes pos=0 and ignores EFWGetPosition error. An error without an output write becomes slot 1 and successful Calibration finished, even though physical completion was never confirmed. Reproduced with EFW_ERROR_REMOVED. Check SDK status before interpreting -1/motion or publishing completion; publish ALERT on read failure. | Closed (fixed and verified, 2026-09-07) |
| DRV-058 | Medium | `wheel_asi/indigo_wheel_asi.driver:248`; `wheel_asi/indigo_wheel_asi.c:212` | Incomplete asynchronous calibration completion: START=false is accepted and dispatch sets X_CALIBRATE BUSY, but handler neither updates it nor schedules completion. Disconnect during active calibration also cancels its finalizer without resetting X_CALIBRATE state/switch; reconnect redefines that same BUSY property and further changes are blocked. START=false remaining BUSY across reconnect reproduced; interrupted-calibration variant checked statically. Complete no-op requests and reset interrupted calibration state during lifecycle cleanup. | Closed (fixed and verified, 2026-09-07) |
| DRV-059 | Medium | `ccd_touptek/indigo_ccd_touptek.c:1151`, `ccd_touptek/indigo_ccd_touptek.c:1911`, `ccd_touptek/indigo_ccd_touptek.c:2341` | Existing disconnect close guards skip Close for a CCD without a guider, and for standalone wheel/focuser devices whose private camera pointer refers to themselves: their own gp_bits remains 1 until after the guard. These paths can retain the SDK handle/global lock after ordinary disconnect. | Closed (fixed in step 2; SDK replacement validation, hardware pending) |
| DRV-060 | Medium | `ccd_touptek/indigo_ccd_touptek.c:1789`, `ccd_touptek/indigo_ccd_touptek.c:1894` | Wheel calibration and connection initialization wait in unbounded one-second polling loops while SDK position is -1. Moving these bodies unchanged onto a persistent lifecycle/device queue can prevent queued disconnect/removal from progressing. Preserve the operation sequence but provide cancellable delayed completion when migrating these paths. | Closed (fixed in step 2; SDK replacement validation, hardware pending) |
| DRV-061 | High | `guider_cgusbst4/indigo_guider_cgusbst4.driver` | RA completion cleared WEST twice and left EAST nonzero; completion helpers lacked `_finalizer`, causing the generated OK epilogue to prematurely finish pulses. | Closed (fixed) |
| DRV-062 | Medium | `aux_geoptikflat/indigo_aux_geoptikflat.driver` | Successful ping followed by failed firmware query leaked the serial handle. | Closed (fixed) |
| DRV-063 | Medium | `aux_rts/indigo_aux_rts.driver` | RTS control errors were ignored and exposures reported success when the output failed. | Closed (fixed) |
| DRV-064 | Withdrawn | `wheel_manual/indigo_wheel_manual.driver` | The proposed NaN/fractional/range checks duplicated the framework input contract. | Closed (withdrawn) |
| DRV-065 | High | `focuser_astromechanics/indigo_focuser_astromechanics.driver` | Motion readback overwrote the requested target before comparing it, so any first reply was treated as completion. | Closed (fixed) |
| DRV-066 | High | `focuser_usbv3/indigo_focuser_usbv3.driver` | Both move handlers blocked the queue in unbounded read loops and ignored command/read failures. | Closed (fixed) |
| DRV-067 | High | `mount_synscan/indigo_mount_synscan.driver` | An already-at-target slew skipped stopping an actively tracking axis, so HOME could wait indefinitely. | Closed (fixed) |
| DRV-068 | High | `aux_dsusb/indigo_aux_dsusb.driver` | Start/focus/stop errors were ignored, focus blocked the queue and the configuration switch had no change handler. | Closed (fixed) |
| DRV-069 | High | `focuser_fcusb/indigo_focuser_fcusb.driver` | Timed motion blocked the queue and ignored SDK errors; frequency changes had no handler. | Closed (fixed) |
| DRV-070 | High | `guider_gpusb/indigo_guider_gpusb.driver` | RA completion cleared WEST twice; missing finalizer convention caused premature OK and SDK failures were ignored. | Closed (fixed) |
| DRV-071 | High | `wheel_sx/indigo_wheel_sx.driver`, `wheel_atik/indigo_wheel_atik.driver` | Move/poll errors were ignored and motion could poll indefinitely; invalid slot/count readback was accepted. | Closed (fixed) |
| DRV-072 | High | `aux_upb/indigo_aux_upb.driver` | A GOTO published its target as measured position before motor readback; abort consequently reported the unvisited target. | Closed (fixed) |
| DRV-073 | Medium | `rotator_simulator/indigo_rotator_simulator.driver` | Disconnect canceled the timer but retained an unfinished target; the generated reconnect timer resumed the old move. | Closed (fixed) |
| DRV-074 | High | `polaralign_simulator/indigo_polaralign_simulator.c` | Offset changes never assigned the private motion targets, so only zero/no-op tests passed. | Closed (fixed) |
| DRV-075 | High | `dome_simulator/indigo_dome_simulator.driver` | Legacy untracked motion timers survived generated queue teardown; shutter handling blocked the queue for six seconds. | Closed (fixed) |
| DRV-076 | High | `gps_nmea/indigo_gps_nmea.driver` | NMEA parsing ran on an uninitialized buffer after read failure, unbounded comma splitting could overrun the token array and short known sentences dereferenced missing fields. | Closed (fixed) |
| DRV-077 | Medium | `mount_synscan/indigo_mount_synscan.driver` | PPEC training used untracked timers outside the generated queue, allowing post-disconnect transport access. | Closed (fixed) |
| DRV-078 | High | `mount_simulator/indigo_mount_simulator.c` | Mount disconnect did not cancel the manual-motion timer; guider timers were canceled on connect instead of disconnect. | Closed (fixed) |
| DRV-079 | High | `ao_sx/indigo_ao_sx.driver` | Guider RA used AO property macros and omitted the 10 ms duration conversion. | Closed (fixed) |
| DRV-080 | High | `wheel_indigo/indigo_wheel_indigo.driver` | Slot motion slept/polled inside the handler for up to 30 seconds, blocking queued disconnect. | Closed (fixed) |
| DRV-081 | High | `aux_sqm/indigo_aux_sqm.driver` | A truncated `r` sensor record passes NULL from `strtok_r` into `indigo_atod` and crashes (fake transport exit 139). | Closed (fixed) |
| DRV-082 | High | `aux_skyalert/indigo_aux_skyalert.driver` | Negative read results were treated as success and a partial record still connected. | Closed (fixed) |
| DRV-083 | Medium | `wheel_sx`, `wheel_atik`, `wheel_indigo`, `aux_dsusb`, `focuser_fcusb`, `aux_upb`, `dome_simulator`, `focuser_astromechanics` `.driver` files | User review identified duplicated framework BUSY/input guards and redundant generator-owned updates. | Closed (fixed) |
| DRV-084 | High | `guider_gpusb/indigo_guider_gpusb.driver` | Replacement canceled the old stop before the SDK accepted the new pulse; failure could leave a relay on. | Closed (fixed) |
| DRV-085 | Medium | `aux_cloudwatcher/indigo_aux_cloudwatcher.c:719` | Low-resolution humidity uses the temperature coefficient and reports negative humidity for valid sensor data. | Open |
| DRV-086 | High | `aux_cloudwatcher/indigo_aux_cloudwatcher.c:376` | A serial timeout before the first reply byte writes response[-1]. | Open |
| DRV-087 | High | `aux_dragonfly/shared/dragonfly_shared.c:121` | A full-size UDP reply writes the terminator one byte past the response buffer. | Open |
| DRV-088 | High | `aux_mgbox/indigo_aux_mgbox.c:204` | Truncated checksummed NMEA sentences dereference missing fields in GPS and weather parsing. | Closed (fixed) |
| DRV-089 | High | `aux_mgbox/indigo_aux_mgbox.c:195` | Pointer handles are tested as integer descriptors; failed open and last disconnect do not complete correctly. | Closed (fixed) |
| DRV-090 | High | `guider_asi/indigo_guider_asi.c:266` | SDK pulse start/stop errors are ignored in property state and completion. | Open |
| DRV-091 | High | `guider_asi/indigo_guider_asi.c:273` | Reversing a running pulse enables the opposite relay without disabling the original direction. | Open |
| DRV-092 | High | `guider_asi/indigo_guider_asi.c:259` | A zero-duration replacement cancels the stop timer but never switches off the active relay. | Open |
| DRV-093 | Medium | `guider_asi/indigo_guider_asi.c:522` | Failed hot-plug registration leaves INIT recorded as successful, so retry skips registration. | Open |
| DRV-094 | High | `guider_asi/indigo_guider_asi.c:472` | An already scheduled arrival callback can attach a device after SHUTDOWN returns. | Open |
| DRV-095 | Medium | `guider_cgusbst4/indigo_guider_cgusbst4.driver:110` | Direction encoding differs from upstream PHD2 (letters versus digits); device-protocol compatibility needs confirmation. | Open (protocol confirmation needed) |
| DRV-096 | High | `focuser_lunatico/shared/lunatico_shared.c:336` | A full-size serial/UDP reply overflows the response terminator; reproduced through rotator_lunatico with ASan. | Open |
| DRV-097 | Medium | `focuser_lunatico/shared/lunatico_shared.c:1543` | Rejected rotator GOTO never publishes ALERT and idle polling reports OK instead. | Open |
| DRV-098 | High | `focuser_lacerta/indigo_focuser_lacerta.c:88` | Full response writes its terminator beyond the buffer; ASan reproduced. | Closed (fixed) |
| DRV-099 | High | `focuser_lacerta/indigo_focuser_lacerta.c:59` | Missing/malformed identity and transaction failures are accepted or block connection. | Closed (fixed) |
| DRV-100 | Medium | `focuser_lacerta/indigo_focuser_lacerta.c:153` | No-op motion remains BUSY; abort/completion ownership needs correction. | Closed (fixed) |
| DRV-101 | High | `focuser_ioptron/indigo_focuser_ioptron.driver:86` | Legacy status parser accepted invalid moving flags and connection did not require a valid initial status. Reproduced by init_status and poll_badflag. | Closed (fixed) |
| DRV-102 | Medium | `focuser_ioptron/indigo_focuser_ioptron.driver:318` | Legacy custom ZERO_SYNC lacked the mandatory X_ prefix; generated property is X_FOCUSER_ZERO_SYNC. | Closed (fixed) |
| DRV-103 | High | `focuser_efa/indigo_focuser_efa.driver:56` | Legacy reply length overflowed a 16-byte stack buffer and checksum was ignored. Both reproduced; bounded validated frames fix the transport. | Closed (fixed) |
| DRV-104 | Medium | `focuser_efa/indigo_focuser_efa.driver:224` | Temperature parsing lacked shape/NC validation and simulator encoded an unrealistic sample. Explicit signed formats and NC handling preserve valid readings. | Closed (fixed) |
| DRV-105 | High | `focuser_prodigy/indigo_focuser_prodigy.driver` | Legacy logical connections overwrite the shared handle and initialize the same mutex twice. Generated master queue/reference ownership and transactional rollback preserve peer sessions. | Closed (fixed) |
| DRV-106 | Medium | `focuser_prodigy/indigo_focuser_prodigy.driver` | Legacy accepts arbitrary OK_ identities and ignores malformed/failed command replies; second power outlet incorrectly decodes 2 instead of documented boolean 1. Validated transport/ACK/readback and simulator fixtures correct these paths. | Closed (fixed) |
| DRV-107 | High | `ccd_sx/indigo_ccd_sx.driver`, `CCD_ABORT_EXPOSURE.on_change` and other generated abort handlers | URGENT abort could overtake queued NORMAL operation starts, allowing motion or exposure to start after abort completed. All 27 affected abort branches now cancel the associated pending starts. | Closed (fixed) |
| DRV-108 | Medium | `aux_dsusb/indigo_aux_dsusb.driver`, `CCD_ABORT_EXPOSURE.on_change` and other generated abort handlers | Initial pending-start cancellation prologues bypassed existing abort-switch/BUSY guards. Cancellation now follows each existing abort condition; DSUSB and RTS barrier tests verify false abort preservation. | Closed (fixed) |
| DRV-109 | High | `dome_skyroof/indigo_dome_skyroof.driver`, `DOME_ABORT_MOTION.on_change` and other generated abort handlers | Canceling a queued start prevents its completion callback from being scheduled, leaving BUSY properties unresolved where abort relied solely on that callback. Abort now explicitly settles pending operations or schedules the existing completion path. | Closed (fixed) |
| DRV-110 | Medium | `ccd_playerone/indigo_ccd_playerone.driver:1116` | Idle abort between exposures changed state and reset the switch without publishing a terminal update because the acquisition_finalizer reference suppresses the generated epilogue. Clients retained BUSY. The idle branch now explicitly publishes ALERT; SDK regression tests and physical preview/streaming trials passed. | Closed (fixed) |
| DRV-111 | High | `../indigo_libs/indigo_filter.c:361`, `../indigo_libs/indigo_filter.c:621` | `independent instances and reselection`: removing instance #2 frees its client without clearing the client pointer; subsequent agent shutdown frees it again. ASan confirms double-free. Clearing the client slot immediately after freeing it fixes the repeated release; the original regression passes normally and with ASan. | Closed (fixed) |
| DRV-112 | Medium | `agent_imager/indigo_agent_imager.c:1397` | `exposure failure retry exhaustion and recovery`: after three camera exposure failures the retry loop fell through and the batch published success. The batch now returns failure when all three attempts are exhausted. Regression failed before the fix and passes afterward: third-attempt success, exhaustion for one-frame/multi-frame/infinite batches, no further images after exhaustion and successful subsequent acquisition. Finite acquisition and abort/reacquire cases also pass. | Closed (fixed) |
| DRV-113 | Medium | `agent_imager/indigo_agent_imager.c:1616` | `streaming failure propagates`: a camera stream finishing ALERT was reported as successful by the agent. streaming_batch now succeeds only when the camera finishes OK. The regression failed before the fix and passes afterward for counts 1, 3 and -1, including cleared start switch, no images from failed streams and successful three-frame recovery after each failure. Finite acquisition, abort/reacquire and pause/resume cases also pass. | Closed (fixed) |
| DRV-114 | Medium | `agent_imager/indigo_agent_imager.c:2434` | `external shutter routing`: the auxiliary branch sent CCD_ABORT_EXPOSURE again instead of AUX_1_CCD_ABORT_EXPOSURE, so the shutter received no abort. The branch now targets the AUX_1 property. Regression failed before the fix and passes afterward for preview and exposure batch: exactly one abort each to camera and shutter, shutter terminal state/switch reset, reacquisition, and no shutter requests after deselection. General abort/reacquire and two-agent barrier cases also pass. | Closed (fixed) |
| DRV-115 | Medium | `agent_imager/indigo_agent_imager.c:3737` | `camera disconnect and recover`: deleting the selected camera exposure property left its cached state BUSY, preventing preview completion. Deletion now sets the corresponding exposure/streaming cache to ALERT, including deletion of all properties; independent deletion checks also preserve local-folder cleanup. Regression failed before the fix and passes afterward for preview, exposure batch and streaming, each followed by reconnection and acquisition. Existing retry, streaming-failure and abort/reacquire cases pass. | Closed (fixed) |
| DRV-116 | Medium | `agent_imager/indigo_agent_imager.c:490` | `additional instances and barrier`: validate_related_agent excludes Imager Agent, so another real instance never appears as a selectable related agent. Fixed by admitting the Imager Agent prefix. The real two-agent test now passes breakpoint propagation, barrier resume, one image per camera, propagated abort and shutdown. | Closed (fixed) |
| DRV-117 | Medium | `agent_imager/indigo_agent_imager.c`, `exposure_batch()` | TRIGGER plus any breakpoint marked the instance controlled and skipped both delay breakpoints. The controlled-instance guard now suppresses only dithering, while delay and its breakpoints execute normally. Abort checks after PRE_DELAY and POST_DELAY also handle zero delay and the final frame. Both breakpoint regressions pass for one/two frames and zero/nonzero delay, with resume, abort and reacquisition. Dithering, capture breakpoints and two-agent barrier tests also pass. | Closed (fixed) |
| DRV-118 | Medium | `agent_imager/indigo_agent_imager.c:1405` | `breakpoint POST_BATCH resume and abort`: abort released the final breakpoint, but exposure_batch returned true and bypassed abort finalization, leaving AGENT_ABORT_PROCESS BUSY. The function now returns failure when abort is pending after POST_BATCH. Regression failed before the fix and passes afterward for one/two frames, normal resume, abort with breakpoint still enabled, terminal states/switch reset, no extra requests/images and subsequent batch. All six breakpoint cases, two-agent barrier and general abort/reacquire pass. | Closed (fixed) |
| DRV-119 | Medium | `agent_imager/indigo_agent_imager.c:2583` | With fresh configuration the U-Curve switch was selected but its HFD/algorithm flags remained false until an explicit estimator change. Attach now initializes both flags alongside the default switch. Corrected regression selects a star without changing estimator: it fails without the fix and passes with it, including HFD and autofocus convergence. Saved RMS configuration overrides the defaults correctly; explicit estimator and Bahtinov tests also pass. | Closed (fixed) |
| DRV-120 | Medium | `agent_imager/indigo_agent_imager.c`, `additional_instances_handler()` / `ADDITIONAL_INSTANCES` change branch | Synchronous instance removal held the bus mutex while indigo_cancel_pending_handlers waited for a disk-usage callback trying to publish through that same mutex. Thread sampling confirmed the cycle. Instance changes now run on the existing agent queue using the standard BUSY guard; configuration is saved after the change. A forced publication/removal regression times out without the fix and passes normally and with ASan. All four instance cases and ten repeated lifecycle/publication/barrier runs pass; the complete normal suite passes 37/37 including cleanup. | Closed (fixed) |
| DRV-121 | Low | `ccd_asi/indigo_ccd_asi.c:590` | Streaming countdown decrements the fractional exposure value by one per sleep and publishes it without rounding (e.g. 3.5 s produces 2.5 and 1.5 s). Derive the display from a monotonic deadline and publish whole seconds rounded upward, independently of SDK acquisition timing. Static finding; hardware reproduction pending. | Open |
| DRV-122 | Low | `ccd_svb/indigo_ccd_svb.c:582` | Streaming countdown subtracts integer wall-clock seconds from the exact exposure target and publishes the fractional remainder without rounding. `time(NULL)` also gives coarse elapsed time and is affected by wall-clock adjustments. Use a monotonic deadline and round only the displayed remainder upward, preserving the requested SDK exposure duration. Static finding; hardware reproduction pending. | Open |
| DRV-123 | Low | `ccd_ptp/indigo_ptp_canon.c:1637` | The legacy Canon BulbStart/BulbEnd branch publishes a fractional countdown by decrementing `CCD_EXPOSURE_ITEM->number.value`; the same value controls the final sleep before closing the shutter. Separate the exact monotonic shutter deadline from the rounded display before changing countdown publication, otherwise rounding can lengthen the exposure. The separate `ptp_blob_exposure_timer()` already rounds its display. Static finding; hardware reproduction pending. | Open |
| DRV-124 | High | `agent_guider/indigo_agent_guider.c:728` | RAW header, dimensions and pixel payload are now validated before image access, with division checks avoiding overflow and signed-int limits matching image/Bayer helpers. Combined fix version is now `0x0300002D`; all four RAW integration cases pass normally and under AddressSanitizer, including short headers, invalid dimensions, all four pixel formats, Bayer metadata and recovery. | Closed (fixed) |
| DRV-125 | High | `agent_guider/indigo_agent_guider.c:2542` | Active work is stopped/joined before filter-client/cache teardown on SHUTDOWN and additional-instance removal. Removal runs outside the bus change callback lock; configuration loading retains client-owned initial attachment. Shutdown, subframe restoration, exposure abort, active-instance removal/sibling survival and restart pass normally and under AddressSanitizer. Combined fix version is now `0x0300002D` (DRV-130 remains deferred). | Closed (fixed) |
| DRV-126 | Medium | `agent_guider/indigo_agent_guider.c:1259` | Single preview preserves the capture result and abort reason, publishing OK/DONE for success or abort and ALERT/FAILED for acquisition failure. Five focused integration cases pass, covering successful capture, retries, exhausted retries, invalid RAW, format restoration and abort/restart. Combined fix version is now `0x0300002D`. | Closed (fixed) |
| DRV-127 | Medium | `agent_guider/indigo_agent_guider.c:1290` | Continuous preview preserves the abort reason before clearing it: explicit abort publishes OK/DONE, while acquisition failure/disconnection publishes ALERT/FAILED. Six focused integration cases pass, covering abort/restart, exposure failures, malformed RAW, disconnect/reconnect, shutdown and simultaneous agents. Combined fix version is now `0x0300002D`. | Closed (fixed) |
| DRV-128 | Medium | `agent_guider/indigo_agent_guider.c:1304` | Adaptive calibration compares the step with its minimum before reducing it and clamps automatic multiplication/division to the advertised bounds. Six focused integration scenarios pass, including successful reduction, minimum/maximum exhaustion, recovery, no motion, abort and calibration-to-guiding. Combined fix version is now `0x0300002D`. | Closed (fixed) |
| DRV-129 | Medium | `agent_guider/indigo_agent_guider.c:1494` | Calibration uses completed pulse counts for north/west speed, east return speed and backlash tolerance; return legs preserve matching pulse counts. Six focused integration cases pass, including 10 px/s accuracy, a single-pulse RA calibration, asymmetric west/east speed averaging, adaptive step, RA-only calibration and subsequent guiding. Combined fix version is now `0x0300002D`. | Closed (fixed) |
| DRV-130 | Medium | `agent_guider/indigo_agent_guider.c:2637` | RA-only dithering uses `dith_total / (cos_angle + tan_angle)` rather than a unit-vector projection. At 45 degrees, a requested (3,4) offset with magnitude 5 becomes approximately (-17.071,17.071), magnitude 24.142. Preserve magnitude with sine/cosine projection and keep derived values within the intended dither area. Reproduced by `integration/test_agent_guider.c`: dither RA projection preserves magnitude. | Open |
| DRV-131 | Medium | `agent_guider/indigo_agent_guider.c:2059` | The inter-frame delay checks abort on each existing sleep iteration, then clears delay and follows normal guiding finalization. Four focused cases pass, including abort during 4 s/0.5 s delays, shutdown during delay, no subsequent exposure and restart. Combined fix version is now `0x0300002D`. | Closed (fixed) |
| DRV-132 | Medium | `agent_guider/indigo_agent_guider.c:1226` | Pulse completion now requires OK from each requested axis; ALERT or remaining BUSY after the existing timeout propagates failure. Unused-axis states are ignored, abort interrupts the completion wait, and calibration marks failure without launching subsequent guiding. CONTINUE keeps guiding active after a pulse error, pauses briefly and computes the next correction from a fresh frame; FAIL terminates. Transient RA/DEC recovery tests verify successful corrections without restarting guiding. Combined fix version is now `0x0300002D`. | Closed (fixed) |
| DRV-133 | Medium | `agent_guider/indigo_agent_guider.c:1819` | Every accepted guiding frame now updates drift/correction statistics, correction histories and PPEC, including exact zero drift. Regression checks cover displacement returning to zero, decreasing short-term RMSE, no unnecessary PI pulses and PPEC learning on zero drift. Driver version is `0x0300002D` for the combined fixes; deferred DRV-130 remains open. | Closed (fixed) |
| DRV-134 | Medium | `agent_mount/indigo_agent_mount.c:1485` | Dome deselection now clears shutter flags, horizontal-coordinate state, all dome state lights and capabilities, and resets `dome_state_defined` to false so a reselected legacy dome can report park/shutter completion. Version `0x03000017`; all three dome-reselection regressions pass, including modern state reporting, plus legacy/modern operation matrices, selection orders and slaving. | Closed (fixed) |
| DRV-135 | Medium | `agent_mount/indigo_agent_mount.c:1094` | Negative imager OBJCTDEC now converts the fractional declination to minutes before truncating to an integer. Expanded `negative fits` covers -12.5, -12.125, -0.125 and -90 degrees, including nonzero seconds and a negative subdegree sign. | Closed (fixed) |
| DRV-136 | Medium | `agent_mount/indigo_agent_mount.c:1105`, `agent_mount/indigo_agent_mount.c:1156` | Guider OBJCTDEC and imager SITELAT/SITELONG now format the sign from the original coordinate separately from the absolute integer degrees, preserving negative subdegree values. Expanded regressions cover negative/positive subdegree and multi-degree coordinates, zero and nonzero seconds, including both site axes. | Closed (fixed) |
| DRV-137 | Medium | `agent_mount/indigo_agent_mount.c:738` | LX200 ACK byte 0x06 now writes its single-byte P reply immediately through indigo_uni_write, instead of leaving it in the output buffer handled only by colon commands. Expanded ACK regression covers single/repeated ACKs and ACKs interleaved with identification, separators and unknown commands. All three targeted LX200 cases pass. | Closed (fixed) |
| DRV-138 | Medium | `agent_mount/indigo_agent_mount.c:788` | LX200 Sd now recognizes a negative degree value or an explicit minus prefix, preserving -00 while decoding +00 as positive in both full and minute-only formats. Expanded `lx200 positive zero` passes 12 signed-coordinate fixtures; the protocol and input-matrix cases also pass. | Closed (fixed) |
| DRV-139 | Medium | `agent_mount/indigo_agent_mount.c:745` | LX200 commands are dispatched only after their terminating # is received. EOF, read errors and oversized frames close the connection without executing a partial command or changing requested coordinates. Expanded `lx200 truncated command` and new `lx200 command length` regressions pass. | Closed (fixed) |
| DRV-140 | Medium | `agent_mount/indigo_agent_mount.c:721` | LX200 Sr/Sd now validate complete coordinate syntax, separators and component ranges before updating requested coordinates. Invalid input returns 0 and retains the previous target. Short RA decimal minutes are decoded correctly. Expanded invalid-input and boundary regressions pass. | Closed (fixed) |
| DRV-141 | Medium | `agent_mount/indigo_agent_mount.c:921` | LX200 startup remains BUSY until the listener-ready callback publishes STARTED/OK. Open/bind failure publishes STOPPED/ALERT. The expanded `lx200 bind failure` regression covers delayed failure, successful retry/stop and repeated failure without stale success. | Closed (fixed) |
| DRV-142 | Medium | `agent_mount/indigo_agent_mount.c:938` | STOPPED with no listener now explicitly publishes STOPPED/OK instead of leaving the request BUSY. Expanded `lx200 idle stop` passes initial/repeated stop, stop after failed startup, successful restart/stop and repeated stop after listener closure, with no redundant socket closes. | Closed (fixed) |
| DRV-143 | High | `agent_mount/indigo_agent_mount.c:2325` | Detach now closes the active LX200 listener before indigo_cancel_all_timers waits for its blocking timer callback. Expanded `shutdown server` passes three start/shutdown/reinitialization cycles, verifying one listener close and callback completion before shutdown returns. | Closed (fixed) |
| DRV-144 | High | `agent_mount/indigo_agent_mount.c:2459` | SHUTDOWN now requests the existing agent ABORT for a BUSY process and cancels/joins handlers before detaching the internal client. Expanded `shutdown active` passes nine operation/shutdown/reinitialization cycles, verifies abort forwarding to mount/dome/rotator and no unsolicited abort on idle shutdown. | Closed (fixed) |
| DRV-145 | Medium | `agent_mount/indigo_agent_mount.c:2206`, `agent_imager/indigo_agent_imager.c:3013`, `agent_guider/indigo_agent_guider.c:2835`, `../indigo_libs/indigo_platesolver.c:1048` | Empty AGENT_START_PROCESS requests now complete directly with OK in Mount, Imager, Guider and the shared platesolver (ASTAP/Astrometry). Previously Imager with camera/focuser and Guider with camera/guider stayed BUSY without an operation; incomplete selections produced misleading missing-device errors. Platesolver instead started an unintended exposure. Empty requests now schedule no work, and BUSY requests preserve the active selection. | Closed (fixed) |
| DRV-146 | Medium | `agent_mount/indigo_agent_mount.c:2360`, `agent_mount/indigo_agent_mount.c:2453` | Capability discovery retained mount/dome feature flags after deletion of the selected device’s supporting property. Removing MOUNT_HOME left HOME advertised and defeated the unsupported-operation guard. Definitions and deletions now recompute capabilities from live filter cache entries and publish changes, preserving alternative dome slew sources. Expanded `stale capability` covers deletion, rejection without commands and restoration. | Closed (fixed) |
| DRV-147 | High | `agent_mount/indigo_agent_mount.c:308`, `agent_mount/indigo_agent_mount.c:332` | Slew/sync sent unpark requests and immediately continued to mode/coordinate commands, even after an immediate unpark ALERT. The shared path now waits for successful required mount/dome unpark before commanding mount, dome or rotator coordinates; failure, abort, lost device/capability or timeout ends START with ALERT. | Closed (fixed) |
| DRV-148 | Medium | `agent_config/indigo_agent_config.c:241` (relevant lines 241–282) | **LOAD reports success when the selected file cannot be opened or contains no valid configuration.** The open failure has no failure assignment and XML parsing has no checked success result. Both a removed .saved file still listed in LOAD and a file containing plain garbage finish LOAD/OK. Set ALERT and preserve a useful last-configuration status when no configuration was read. Regression cases: `missing_file`, `malformed_file`. | Closed (fixed) |
| DRV-149 | Medium | `agent_config/indigo_agent_config.c:119` (relevant lines 119–134) | **Configuration discovery accepts backup files as configurations.** configuration_filter uses strstr rather than an end-of-name suffix match; backup.saved.bak is listed as backup, although loading opens backup.saved. Require the actual .saved suffix and preserve the complete basename. Regression cases: `scan_suffix`. | Closed (fixed) |
| DRV-150 | Medium | `agent_config/indigo_agent_config.c:562` (relevant lines 562–564) | **REMOVE bypasses the portable configuration directory.** SAVE and discovery use indigo_uni_config_folder(), while REMOVE reconstructs HOME/.indigo in a 256-byte buffer. A configuration in an alternate framework directory cannot be removed. The Windows directory mismatch and long-path truncation follow from source inspection; Windows was not run. Resolve removal through the shared configuration path policy. Regression cases: `alternate_folder_remove`. | Closed (fixed) |
| DRV-151 | Medium | `agent_config/indigo_agent_config.c:125` (relevant lines 125–134, 245, 563) | **Non-default service ports produce names that cannot be loaded back.** At port 7625, SAVE(port) writes port_7625.saved; discovery exposes port_7625 and LOAD then appends the port again, looking for port_7625_7625.saved. LOAD returns OK without setting LAST to the requested configuration. Apply the service namespace consistently to discovery, load and removal. Regression cases: `port_namespace`, `nondefault_port_load`. | Closed (fixed) |
| DRV-152 | Medium | `agent_config/indigo_agent_config.c:231` (relevant lines 231–238) | **Deselect timeout leaves LAST without a terminal failure update.** The early return publishes LOAD/ALERT but never finalizes LAST. In the fixture, intermediate related-agent updates leave LAST/IDLE; without such updates it can retain BUSY. Publish LAST/ALERT on this exit as well. Regression cases: `deselection_timeout`. | Closed (fixed) |
| DRV-153 | Medium | `agent_config/indigo_agent_config.c:368` (relevant lines 368–391) | **Exhausted restore polling is reported as success.** After twenty BUSY polls the loop simply ends without setting failure. With a single filter held BUSY (no related property to mask its state), LOAD still finishes OK. Record timeout failure and preserve the incomplete operation status. Regression cases: `restore_busy_timeout`. | Closed (fixed) |
| DRV-154 | Medium | `agent_config/indigo_agent_config.c:324` (relevant lines 324–326) | **Profile change rejection is ignored.** A present device rejecting the saved profile publishes PROFILE/ALERT and retains its old profile, but the agent only checks device presence and dispatches the change. LOAD finishes OK. Confirm profile selection and terminal status. Regression cases: `profile_rejection`. | Closed (fixed) |
| DRV-155 | Medium | `agent_config/indigo_agent_config.c:285` (relevant lines 285–309) | **Server driver-change rejection is ignored.** The server publishes DRIVERS/ALERT in response to the restore request, but the agent neither inspects that status nor sets failure. LOAD finishes OK. Require successful driver restoration before reporting a completed configuration. Regression cases: `driver_rejection`. | Closed (fixed) |
| DRV-156 | Medium | `agent_config/indigo_agent_config.c:371` (relevant lines 371–387) | **Restoration to an absent agent is treated as complete.** The restore wait initializes done=true and never clears it when no matching agent exists. A configuration referring to an absent agent therefore finishes OK. Require discovery and successful restoration of each referenced agent. Regression cases: `absent_agent`. | Closed (fixed) |
| DRV-157 | Medium | `agent_config/indigo_agent_config.c:764` (relevant lines 764–773) | **Whole-server deletion retains the cached driver list.** The delete handler clears drivers only for an explicitly named DRIVERS property, whereas whole-device deletion uses an empty property name. Removing the server leaves its drivers in the configuration snapshot. Handle both deletion forms. Regression cases: `server_disappearance`. | Closed (fixed) |
| DRV-158 | Medium | `agent_config/indigo_agent_config.c:648` (relevant lines 648–655) | **A profile update with no selected item retains the previous profile.** add_profile only writes a value when it finds a selected switch. When a device publishes an all-false profile vector, the snapshot still says Night and can save that obsolete choice. Clear the mirror before deriving the current selection. Regression cases: `profile_no_selection`. | Closed (fixed) |
| DRV-159 | High | `agent_config/indigo_agent_config.c:69` (relevant lines 69, 89, 585–591) | **The restore queue silently drops configuration records at sixteen.** The same MAX_AGENTS limit is used for agent slots and all restore records, including DRIVERS and PROFILES. A saved configuration with sixteen supported agents restores only fourteen; the last two remain deselected while LOAD reports OK. Completed direct restore requests also never reclaim restore_count, so request seventeen is silently ignored until a file load resets it. Use a correctly sized/reclaimed queue and report overflow. Regression cases: `capacity_restore`, `direct_restore_capacity`. | Closed (fixed) |
| DRV-160 | High | `agent_config/indigo_agent_config.c:487` (relevant lines 487–500, 627–657) | **Autosave deadlocks on a synchronous device profile update.** SAVE holds data_mutex while sending CONFIG/SAVE on the local bus. A target device publishing a changed PROFILE synchronously reenters add_profile and waits for the same nonrecursive mutex. The isolated regression is terminated by its five-second watchdog (SIGALRM); normal autosave without that callback passes. Copy the needed targets under the lock and dispatch after unlocking. Regression cases: `autosave_reentrant`. | Closed (fixed) |
| DRV-161 | High | `agent_config/indigo_agent_config.c:532` (relevant lines 532–534, 542–550, 571–573) | **SAVE and REMOVE replace the active LOAD selection.** While a LOAD worker is paused before opening its selected file, SAVE or REMOVE rebuilds the shared LOAD vector with every item false. The worker then skips the original request and reports OK. SAVE leaves LAST naming the newly saved configuration rather than the requested load. Preserve operation-owned selection and serialize/reject conflicting mutations; the test uses a barrier, not a timing race. Regression cases: `concurrent_save`, `concurrent_remove`. | Closed (fixed) |
| DRV-162 | High | `agent_config/indigo_agent_config.c:505` (relevant lines 505–523) | **Serialization failure still publishes SAVE/OK.** All indigo_save_property return values are discarded. Injecting INDIGO_FAILED at that API after the output file opens leaves an empty saved file and reports SAVE/OK. Existing configurations are opened for overwrite, so real write failures can destroy a previously valid snapshot. Check serialization results and make replacement transactional. Regression cases: `save_write_failure`. | Open (reproduced) |
| DRV-163 | Medium | `agent_config/indigo_agent_config.c:542` (relevant lines 542–550, 172–204) | **An all-false LOAD request deselects the live configuration.** Even with no selected configuration, LOAD schedules load_configuration, deselects the active camera and related agents, and clears LAST before reporting success. Treat an empty selection as a no-op or explicit error before mutating the live setup. Regression cases: `empty_load`. | Open (reproduced) |
| DRV-164 | High | `agent_config/indigo_agent_config.c:481` (relevant lines 481–504) | **SAVE allows a name to escape the configuration directory.** SAVE only replaces whitespace and passes ../outside to indigo_open_config_file, which constructs a path relative to the configuration directory. The isolated request succeeds and writes outside .indigo, within the test-owned temporary root. REMOVE already rejects forward slashes. Apply consistent basename validation before opening any output; no real user configuration was touched. Regression cases: `save_path_separator`. | Open (reproduced) |
| DRV-165 | Low | `agent_config/indigo_agent_config.c:744` (relevant lines 744, 757, 790) | **Update and delete match a different filter prefix than define.** Definitions require the seven-character FILTER_ prefix, but update/delete compare only six characters. A changed FILTERX_CCD_LIST update creates a configuration mirror even though its definition was ignored. Match FILTER_ consistently. Regression cases: `filter_prefix_consistency`. | Open (reproduced) |
| DRV-166 | Medium | `agent_config/indigo_agent_config.c:104` (relevant lines 104–116, 475–480) | **SETUP reports OK when its automatic persistence fails.** A non-writable configuration destination makes save_config set hidden CONFIG/ALERT, but the visible SETUP property still publishes OK. The user cannot tell that the setting was not persisted from its result. Propagate persistence failure to the visible SETUP acknowledgement. Regression cases: `setup_save_failure`. | Open (reproduced) |
| DRV-167 | High | `agent_config/indigo_agent_config.c:368` (relevant lines 368–387, 731) | **A later successful filter update hides an earlier selection failure.** The mirror has one state, overwritten by each constituent filter property. A rejected camera selection sets ALERT, but successful related-agent restoration then sets that same mirror OK; LOAD reports success with no camera selected. The isolated camera-only rejection test correctly reports ALERT. Track per-property restoration results rather than the last update for the entire agent. Regression cases: `selection_alert_masked`. | Open (reproduced) |
| DRV-168 | Medium | `agent_config/indigo_agent_config.c:290` (relevant lines 290–309) | **Unload-unused policy does not disable drivers absent from the saved list.** After saving, introduce a fourth enabled server driver and enable UNLOAD_UNUSED_DRIVERS. Restore only sends the three saved items; the new driver remains enabled although it is unused by the configuration. Build the restore command against the current complete driver inventory when unloading is requested. Regression cases: `driver_unload_new`. | Open (reproduced) |

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

### DRV-025 (Closed)

`ccd_svb` uses `indigo_device_enumeration_mutex` in hot-plug paths around `SVBGetNumOfConnectedCameras()`, `SVBGetCameraInfo()`, temporary `SVBOpenCamera()`, property/probing calls, and `SVBCloseCamera()`. Normal `svb_open()` / close paths use only `PRIVATE_DATA->usb_mutex` around `SVBOpenCamera()` and later `SVBCloseCamera()`. A connect racing with an arrival/removal timer can therefore overlap SDK enumeration and open/close state.

Fixed by reusing `indigo_device_enumeration_mutex` around normal `SVBOpenCamera()` / `SVBCloseCamera()` paths and detaching hot-unplugged devices outside that mutex to avoid self-deadlock when detach invokes close.

### DRV-026 (Closed)

`ccd_touptek` uses a driver-global `mutex` for `process_plug_event()` while refreshing the device inventory with `SDK_CALL(EnumV2)` and updating shared `devices[]` / `present` state. The CCD, guider, wheel, and focuser connect callbacks call `SDK_CALL(Open)` directly from normal connect paths without taking that mutex. If the vendor hot-plug callback schedules a refresh during user connect/disconnect, SDK enumeration and open can overlap.

Fixed by reusing `indigo_device_enumeration_mutex` around normal CCD, guider, wheel, and focuser `SDK_CALL(Open)` / final `SDK_CALL(Close)` paths. Hot-unplug detaches devices outside that mutex to avoid self-deadlock when detach invokes close.

### DRV-027 (Closed)

`ccd_dsi` hot-plug uses `indigo_device_enumeration_mutex` around `dsi_scan_usb()` and, on non-macOS paths, a temporary `dsi_open_camera()` used to name/probe the camera. Normal connect entered `camera_open(device)` outside that mutex. That left the same scan/open race class as the ASI wheel issue, even though macOS avoids the temporary open inside plug handling because it can reset the device.

Fixed by renaming the driver-global hot-plug mutex to `indigo_device_enumeration_mutex` and reusing it around normal `dsi_open_camera()` / `dsi_close_camera()` paths. Hot-unplug detaches devices outside that mutex to avoid self-deadlock when detach invokes close.

### DRV-028 (Closed)

`ccd_qsi` uses a single global `QSICamera cam` object. `process_plug_event()` protects `cam.get_AvailableCameras()` with `indigo_device_enumeration_mutex`, but `ccd_connect_callback()` used the same `cam` object for `get_Connected()`, `get_SelectCamera()`, and the subsequent connect sequence without holding that mutex. Because the SDK object is shared across all QSI devices, hot-plug enumeration could race connect-time SDK state.

Fixed by renaming the driver-global hot-plug mutex to `indigo_device_enumeration_mutex` and reusing it around normal connect/disconnect SDK access to `cam`. Hot-unplug detaches devices outside that mutex to avoid self-deadlock when detach invokes disconnect.

### DRV-029 (Closed)

`guider_asi` has a direct mutex leak in `process_plug_event()`: it locks `indigo_device_enumeration_mutex`, handles error returns correctly, but the successful path attaches the new guider and stores it in `devices[slot]` without unlocking before returning. After the first successful plug event, later ASI USB-ST4 plug/unplug handlers block forever on the same mutex. The fix is a straightforward unlock on the success path, plus considering the DRV-022 serialization fix for normal open/close.

Fixed by unlocking `indigo_device_enumeration_mutex` on the successful attach path.

### DRV-030 (Closed)

`ccd_connect_callback()` in `indigo_ccd_touptek.c` enters the disconnect branch (`else`) and immediately calls `SDK_CALL(Stop)(PRIVATE_DATA->handle)` at line 1123. This runs before any NULL-handle guard. A handle of NULL can result from a prior connect where `SDK_CALL(Open)` returned NULL: the DRV-026 fix correctly releases the global lock in that path, but `PRIVATE_DATA->handle` remains NULL and `gp_bits` is cleared to 0 (line 1120). If the user or a client subsequently sends a disconnect request, `SDK_CALL(Stop)(NULL)` is invoked, which is expected to dereference the NULL handle in the vendor SDK and crash the process.

Fixed by wrapping the `SDK_CALL(Stop)` call with `if (PRIVATE_DATA->handle)`, matching the pattern already used for the `SDK_CALL(Close)` call at line 1151.

### DRV-031 (Closed)

The root cause was in `indigo_generator.c`, not in iOptron-specific source. For generated
multi-device drivers, the connection-handler template emitted
`if (PRIVATE_DATA->count++ == 0) ..._open(...)`, then emitted the driver-specific
`on_connect` block, but the failure branch only emitted `PRIVATE_DATA->count--`. If
`..._open()` succeeded and the generated `on_connect` initialization failed, the generated
handler marked `CONNECTION` disconnected without closing the handle opened by that same
attempt.

`mount_ioptron` exposed the issue because its generated mount and guider handlers both
call `ioptron_init_mount()` / `ioptron_init_guider()` after opening the shared serial/TCP
handle. A rejected or malformed probe could therefore leak the handle and leave the
controller endpoint occupied. Other generated multi-device drivers with the same
open-then-init shape were susceptible to the same template bug.

Fixed in the generator by emitting `if (--PRIVATE_DATA->count == 0) { ..._close(device); }`
on generated multi-device connection failure. The checked-in `mount_ioptron` generated
output was regenerated with that template, so its mount and guider handlers now close the
shared handle only when the failed attempt brings the shared count back to zero.

### DRV-032 (Closed)

For V2.5/V3 controllers, `ioptron_set_guide_rate()` sends both axes in one fixed-width
payload with `:RG%02d%02d#`, and initialization parses `:AG#` as two characters for DEC
and two characters for RA. The inherited mount `MOUNT_GUIDE_RATE` items are initialized
with max 100 in `indigo_mount_driver.c`, and the iOptron driver does not lower that max
when it exposes the two-axis V2.5/V3 form.

As a result, a valid INDIGO-side value of 100 formats as `RG100100`, which is longer than
the two-two digit command shape. The simulator currently masks this by copying only the
first four payload characters, and the integration test exercises 50 but not the 100
boundary.

Fixed by setting the iOptron guide-rate item ranges after protocol detection according to
the local protocol PDFs: V1.0 uses 10..80, V2.0 uses 10..90, and V2.5/V3 use separate
RA 1..90 and DEC 10..99 ranges. The 8407 branch keeps a 10..100 item range because its
documentation permits 10..90 plus 100, while INDIGO number ranges cannot represent the
91..99 gap; the command helper now rejects that gap before sending the protocol command.
The generated driver was regenerated from the updated `.driver` source, and the iOptron
simulator integration test now asserts the V3 mount/guider RA and DEC ranges.

### DRV-033 (Closed)

In `ioptron_set_tracking_rate()`, non-8406 protocols correctly require `*response == '1'`
for the `:RT*#` command and for custom `:RR*#` commands, but the final `:ST1#` command is
combined as `result = result && ioptron_simple_reply_command(device, ":ST1#")`. That
helper returns true when one byte is read, regardless of whether the byte is the success
ack.

`ioptron_set_tracking()` checks `:ST%c#` with `*response == '1'`, so the tracking-rate
path is inconsistent with the direct tracking path. If the controller accepts the rate
selection but rejects enabling tracking, `MOUNT_TRACK_RATE` can still report
`INDIGO_OK_STATE`.

Fixed by requiring the final `:ST1#` reply byte to be `'1'` in the generated
`ioptron_set_tracking_rate()` source and regenerating the checked-in driver output.

### DRV-034 (Closed)

The LX200 driver is hand-written but uses the same shared-connection pattern as generated
multi-device drivers: mount, guider, focuser, and AUX all increment
`PRIVATE_DATA->device_count` before the first logical device opens the serial/TCP handle.
On autodetect failure each connect handler calls `meade_close()`, and `meade_close()`
sets `PRIVATE_DATA->device_count = 0`. Control then falls through to the common failure
block, which decrements the same counter again.

After one failed autodetect from a cold state, `device_count` becomes `-1`. A later
connect attempt evaluates `if (PRIVATE_DATA->device_count++ == 0)` as false, so it skips
`meade_open()` even though there is no valid handle. The driver then tries detection
against a NULL handle, fails, and repeats the underflow. This can leave the LX200 driver
unable to recover from a transient probe failure without unloading/reloading the driver.

Fixed by removing the direct `meade_close()` calls from autodetect failure handling and
letting the common failure path do the reference-count cleanup. The failure path now
decrements `device_count` once and closes the shared handle only when the count reaches
zero.

### DRV-035 (Closed)

The focuser and AUX logical devices reject mount types they cannot support after the
shared handle has already been opened and autodetected. When that rejected logical device
is the first connected instance, `PRIVATE_DATA->device_count` is 1 and the handler's
unsupported-type branch decrements it to 0, but does not call `meade_close()`.

That leaves `PRIVATE_DATA->handle` open while the shared reference count says no logical
device owns it. The next connection attempt can overwrite or reuse the stale handle state,
and a serial port may remain occupied after the user-visible connection has failed.

Fixed by routing the focuser and AUX unsupported-type branches through the same
reference-count cleanup as other connection failures: decrement once, and close the shared
handle when the rejected first logical device brings `device_count` back to zero.

The new LX200 simulator integration test exercises successful Meade mount/guider/focuser
and OnStep AUX paths, but does not currently inject autodetect failures or unsupported
first-slave combinations. It also drives the new tracking-on change through the `:AP#`
fallback because the simulator's `:GW#` response always starts with `P`, so the new
`:AA#` alt/az branch remains a coverage gap rather than a runtime finding.

### DRV-036 (Closed)

`synscan_open()` accepts editable `DEVICE_PORT` values in the form `synscan://host:port`.
When a colon is present, it declares `char host_name[INDIGO_NAME_SIZE]` and then calls
`strncpy(host_name, host, colon - host)`. Because `colon - host` is derived entirely from
the user-controlled text item, a host segment of `INDIGO_NAME_SIZE` bytes or longer writes
past `host_name` before UDP open is attempted. Even shorter exact-width values are not
explicitly null-terminated before being passed to `indigo_open_udp()`.

The new simulator integration test uses a pseudo-terminal serial path, so it does not
exercise the UDP parser or long `synscan://...` endpoints.

Fixed by checking the host segment length before copying it into `host_name`, explicitly
terminating the copied string, and failing the UDP open safely when the host is too long
for `INDIGO_NAME_SIZE`.

### DRV-037 (Closed)

On a successful SynScan guider connection, `synscan_connect_timer_callback()` starts two
zero-delay callbacks, `guider_timer_callback_ra()` and `guider_timer_callback_dec()`. Both
callbacks enter infinite loops and block in `pthread_cond_wait()` until a guide pulse or
`PRIVATE_DATA->guiding_thread_exit` wakes them.

The guider disconnect path does the opposite of the required shutdown signal: it sets
`guiding_thread_exit = false`, decrements `device_count`, and returns `CONNECTION` OK
without signalling either condition variable or cancelling the timer handles. If a user
disconnects the guider but keeps the driver loaded, the two worker callbacks remain parked
on the condition variables. A later reconnect starts another pair of callbacks sharing the
same pulse fields and condition variables, so guide pulses can be consumed unpredictably
and shutdown may have to wait for stale workers. The current integration test disconnects
only during teardown, so `guider_detach()` later sets `guiding_thread_exit = true` and
masks the ordinary disconnect leak.

Fixed by adding a shared guider-worker shutdown helper used by both disconnect and
detach. It cancels pending pulse timers, sets the exit flag, signals both condition
variables, waits for the active guider worker count to drain, and resets pulse state
before starting a fresh pair of workers on reconnect.

### DRV-038 (Closed — fixed)

The ZWO AM driver shares one serial or TCP handle between the mount and the guider device,
refcounted with `device_count`. It carried both multi-device defects fixed in `mount_lx200`
under DRV-034 and DRV-035, but without the LX200 recovery property.

`meade_close()` resets `device_count = 0` outside its `handle != NULL` guard, which is what
makes the `if (--device_count <= 0) meade_close(...)` idiom self-healing: the decrement may
reach `-1`, but the close puts it back to `0`. `asi_close()` reset the counter inside that
guard, so a negative count was never repaired. Because every open is gated on
`if (PRIVATE_DATA->device_count++ == 0)`, a negative count meant `asi_open()` was never
called again and `handle` stayed `NULL` for the life of the loaded driver.

Three paths reached that state. A failed `asi_detect_mount()` closed the handle in the
handshake failure branch, resetting the count to `0`, and the failure path then decremented
it to `-1`. With the guider already connected the same close pulled the handle out from
under it, leaving the guider reporting connected while every guide pulse failed on a `NULL`
handle. A link lost mid-session took `asi_command()` into `asi_close()` and
`indigo_disconnect_slave_devices()`, after which both disconnect branches tested
`--device_count == 0`, which is false at `-1` and `-2`, so the count was never restored and
reconnecting after replugging the cable was impossible.

Fixed by following the `mount_lx200` pattern: `asi_close()` resets `device_count`
unconditionally, the handshake failure branch no longer closes the shared handle, and all
four connect-failure and disconnect paths use `if (--PRIVATE_DATA->device_count <= 0)` before
closing. `asi_close()` is now always called with `device->master_device` for a correct port
name in the disconnect log.

### DRV-039 (Closed — fixed)

POSITION and STEPS on_change blocks merely queue another callback without a `_finalizer` marker, so generated handlers set and publish OK before issuing EAFMove. Neither callback restores BUSY, and the old dispatch logic that marked both motion properties BUSY and rejected overlapping absolute moves is gone. Clients can treat a still-moving focuser as finished; automatic compensation can also pass its position-state guard during a relative move. Move command setup into on_change, preserve both motion states and validation, and let the actual motion finalizer publish completion, as wheel_asi does.

Resolution: Moved SDK operations from callback wrappers directly into `on_change`. Both motion properties remain BUSY until the motion finalizer confirms completion; command failures are published and abort failure retains polling. The temporary SDK-stub behavioral harness and universal production build passed.

2026-09-07 focused cross-review: compared current wheel_asi definition and generated code with focuser_asi findings DRV-039 through DRV-050 (DRV-051 already fixed). A temporary public-bus SDK-stub harness reproduced successful connection despite failed position read, motion polling incrementing stale cached state to slot 5/OK without any successful read, calibration announcing slot 1/OK after failed read, and START=false leaving calibration BUSY across disconnect/reconnect. Recorded DRV-055 through DRV-058. EFWGetProperty failure and use of uninitialized info.slotNum were checked statically, not exercised with uninitialized counts. SDK documents -1 for a moving wheel; successful -1 readings must remain BUSY, not be interpreted as slot 0. CONFIG delegates to the base wheel driver, motion code is inline with finalizer ownership, calibration-command failure resets its switch, and SDK reservations are already fixed. Focuser temperature/compensation/backlash/abort-specific findings do not directly apply. Temporary harness removed; no production changes or folder baseline advancement.

2026-09-07 focused `focuser_asi` refactoring verification against `wheel_asi`: reviewed the driver definition, generated C/header/main, and incremental changes from the recorded baseline through `a35e4ce49483ac2f3cba7b69c20372c2f93a63f3` plus working-tree changes. The SDK hot-plug/open/close structure follows the wheel pattern. Regenerating in a temporary directory reproduced all three generated files byte-for-byte. Fresh x86_64 and arm64 compilation and linking of the archive, dynamic library and standalone executable with `Makefile.drv` passed; vendor SDK deployment-target warnings remain. No EAF hardware or dedicated EAF simulator test was run. Recorded DRV-039 and DRV-040; did not advance the folder baseline. The working-tree `_finalizer` comments fix premature completion for six properties, but omit both motion properties. That verification pass left production sources unchanged. In the subsequent user-requested implementation, all eight property callback wrappers were removed and their SDK operations moved directly into on_change. Motion handlers now keep both motion properties BUSY until the polling finalizer confirms stop, reject overlapping moves, validate targets, and publish SDK failures; abort failure retains polling. EAF_BEEP persistence now uses generator metadata and CONFIG falls through to the base driver. Fresh universal build/link and a temporary SDK-stub behavioral harness passed; DRV-039 and DRV-040 are closed in the working tree. Hardware validation remains outstanding.

2026-09-07 follow-up: on the two early returns in focuser_motion_ready: disconnected requests now publish ALERT for POSITION/STEPS, and the active-polling branch explicitly republishes BUSY without terminating the existing motion. The supplied review overstated reconnect persistence: indigo_focuser_change_property resets POSITION/STEPS to OK during disconnect (indigo_libs/indigo_focuser_driver.c:184). The existing finalizer owns completion of active motion; assigning a terminal state merely because another request is rejected would be incorrect.

### DRV-040 (Closed — fixed)

The inherited CONFIG block intercepts every configuration request and returns before indigo_focuser_change_property. It only saves EAF_BEEP: LOAD/REMOVE become no-ops, standard focuser properties are not saved, and base-driver save-file finalization and switch reset are skipped despite an OK update. Use persistent metadata for EAF_BEEP and base CONFIG handling, or preserve synchronous pass-through to the base handler.

Resolution: Marked EAF_BEEP persistent in generator metadata and removed the CONFIG interception so requests reach the base focuser handler. The SDK-stub behavioral harness and universal build passed.

### DRV-041 (Closed — fixed)

After EAFStop succeeds, abort cancels polling, clears moving and publishes OK without EAFIsMoving confirmation. The SDK distinguishes EAFStop from EAFStopAndWait, and documents that hand-controller movement cannot be stopped by EAFStop. Keep completion polling until the motor is confirmed stopped; do not accept new motion based only on stop-command success.

Resolution: Abort now retains completion polling until the SDK confirms the motor has stopped. New motion checks SDK state before proceeding. Public-bus SDK-stub regressions passed.

2026-09-07 merged follow-up with the supplied Claude review: fixed DRV-041 through DRV-047 and recorded/fixed DRV-048 through DRV-050 below. Motion polling terminates on read error; new motion requests verify SDK state, and automatic compensation can recover on subsequent valid readings. Abort resets its switch on all paths and waits for confirmed stop. Initialization failure closes the single-device SDK handle explicitly because the current generator does not do that on on_connect failure. Limits/readback use confirmed values, and SDK attach eligibility comes from the generated devices array instead of early ID reservations. The driver uses the generator default capacity of five devices as requested; no custom sdk.unplug block is needed after removing the ID reservations. Generator source unchanged. Universal production build/link and six public-bus SDK-stub regression groups passed. No folder-wide baseline advancement.

2026-09-07 follow-up logical review through `109506ce0`, scoped to the current focuser_asi definition/generated C and relevant SDK/bus/queue contracts. Recorded DRV-041 through DRV-047. A temporary SDK-stub harness reproduced stale motion limits, abort completion without stop confirmation, automatic compensation remaining blocked after a transient move failure, and a removed-device temperature error becoming IDLE while discarding the compensation baseline. Remaining findings are static. No production code or generator changes; folder baseline unchanged.

### DRV-042 (Closed — fixed)

A failed compensation EAFMove sets position ALERT and returns without polling/recovery. Every later compensation call requires position OK, so one transient error disables further automatic attempts until another action clears the state. Distinguish recoverable command failure from an active move and define a recovery path.

Resolution: Automatic compensation can retry after subsequent valid readings instead of remaining disabled by a transient move failure. Public-bus SDK-stub regressions passed.

### DRV-043 (Closed — fixed)

FOCUSER_LIMITS updates the hardware maximum and its own value, but POSITION/STEPS maximums remain the attach-time info.MaxStep. Relative/automatic clamping and absolute validation use those stale maximums. A lowered limit still permits commands the SDK rejects; a raised limit can remain inaccessible. Refresh movement ranges from the effective hardware limit on connect and after setting it.

Resolution: Movement ranges now follow the effective hardware maximum on connection and after a limit change. Public-bus SDK-stub regressions passed.

### DRV-044 (Closed — fixed)

Connect-time required reads only log failures and never clear connection_result. The generated connection handler therefore reports success with stale/zero position or maximum data after EAFOpen succeeded but initialization failed. EAFStepRange failure explicitly overwrites the limit range with zero. Fail or explicitly degrade initialization rather than publishing valid-looking data.

Resolution: Required initialization read failures now fail connection, and the single-device SDK handle is explicitly closed on failed initialization. Public-bus SDK-stub regressions passed.

### DRV-045 (Closed — fixed)

temp starts at -273, so SDK errors that do not write the output, including EAF_ERROR_REMOVED, are reported as an absent sensor/IDLE instead of ALERT. The same failure resets prev_temp even in AUTO, discarding uncompensated temperature change when readings recover. Interpret SDK status before its output and preserve the last valid compensation baseline on transient errors.

Resolution: Temperature processing now checks the SDK status before reading its output and preserves the compensation baseline on transient errors. Public-bus SDK-stub regressions passed.

### DRV-046 (Closed — fixed)

After successful SetMaxStep/SetBacklash, a failed GetMaxStep/GetBacklash only logs an error. The generated handler leaves the property OK and publishes the requested cached value as if readback succeeded. Mark readback failure ALERT and preserve a distinction between requested and confirmed settings.

Resolution: Failed settings readback now reports ALERT instead of presenting the requested setting as confirmed. Public-bus SDK-stub regressions passed.

### DRV-047 (Closed — fixed)

SDK ID reservation occurs before generated attach finds a free slot or succeeds. Failure frees private_data without clearing connected_ids, and unplug cannot clear it because no devices entry exists. The rejected focuser stays skipped on replug. The generated capacity is five rather than the ten listed in REFACTOR.md, making this reachable with a sixth EAF. Fixed by detecting IDs among attached devices; the generator default capacity of five is retained as requested.

Resolution: Removed early SDK ID reservations and detect duplicate IDs among successfully attached devices. The generator default capacity of five is retained; failed attach no longer reserves an unattached ID. Public-bus SDK-stub regressions passed.

### DRV-048 (Closed — fixed)

Supplied Claude finding: polling kept rescheduling every 0.5 seconds on SDK errors while the cached moving flag blocked later requests. Error completion now stops the loop and clears the polling flag; SDK preflight prevents treating that flag as proof the motor stopped.

Resolution: Motion read errors terminate polling and clear the polling flag. New requests check SDK state so clearing that flag does not claim the physical motor stopped. Public-bus SDK-stub regressions passed.

### DRV-049 (Closed — fixed)

Supplied Claude finding: EAFStop failure returned before resetting the momentary abort switch. The switch now resets before the SDK operation on both paths.

Resolution: Reset the momentary abort switch before the SDK operation, covering both success and failure. Public-bus SDK-stub regressions passed.

### DRV-050 (Closed — fixed)

Public-bus regression testing found that coefficient/threshold changes had no driver branch and the base focuser handler did not accept them. Added the generator's empty on_change block so requested compensation settings are copied and acknowledged.

Resolution: Added an empty `on_change` block to accept and acknowledge compensation coefficient and threshold changes through the generator. Public-bus SDK-stub regressions passed.

### DRV-051 (Closed — fixed)

Focused follow-up confirms the same stale SDK ID reservation as DRV-047: connected_ids is set in sdk.plug before generated attach finds a slot or succeeds. Failed attach/capacity exhaustion frees private_data without releasing the reservation; sdk.unplug cannot clear it because no attached device entry exists. Replug continues skipping the ID until driver reinitialization. Use the actually attached devices to detect duplicate SDK IDs, as in the corrected focuser_asi.

Resolution: Removed `connected_ids` reservations, their initialization and the obsolete `sdk.unplug` block. Duplicate detection uses attached devices. The failed-attach retry and capacity exhaustion/release regression failed before the fix and passed afterward; the universal production build passed.

2026-09-07 focused SDK/HIDAPI inventory requested by the user: scanned symbol/string evidence in 163 checked-in SDK libraries, including 32 macOS libraries. `nm` confirms additional embedded HIDAPI definitions in libCAA.a (rotator_asi), libUSB2ST4Conv.a (guider_asi), and libPlayerOnePW.dylib (wheel_playerone). macOS x86_64 disassembly (`otool -arch x86_64 -tvV`) shows a persistent `_hid_mgr`, initialized once and scheduled on `CFRunLoopGetCurrent()`; enumeration pumps the calling thread's run loop and reuses that manager. Their hot-plug callbacks dispatch through indigo_set_timer, whose current implementation creates a new callback thread per firing (indigo_libs/indigo_timer.c:307). Recorded potential stale enumeration findings DRV-052 through DRV-054; hardware reproduction remains outstanding. This is distinct from the early connected_ids reservation bug DRV-051. Astroasis focuser/wheel, FCUSB, GPUSB, DSUSB and Atik wheel macOS SDK archives reference external HIDAPI symbols rather than defining their own implementation; repository HIDAPI creates a fresh manager per enumeration (indigo_libs/externals/hidapi/mac/hid.c:688). No comparable embedded-symbol evidence in the other scanned libraries; absence of symbols/strings does not exclude stripped or renamed implementations. No SDK/driver changes and no folder baseline advancement.

2026-09-07 DRV-051 fix: wheel_asi now checks SDK IDs against the generated attached-device array. Removed connected_ids reservations, their initialization, and the now-unnecessary sdk.unplug block. The default five-device capacity and generator are unchanged. A public-driver SDK-stub regression covers failed attach retry and capacity exhaustion followed by slot release and retry. Universal x86_64/arm64 production compile/link passed. The regression fails on the original driver at failed-attach retry and passes with the fix. No physical EFW hardware was tested; folder baseline unchanged.

### DRV-055 (Closed — fixed)

Same initialization problem as DRV-044: EFWGetProperty status is ignored and uninitialized info.slotNum is copied to property counts/max; EFWGetPosition status is also ignored and connection_result remains true. Invalid counts can exceed allocated item arrays. Failed position read reporting successful connection reproduced; uninitialized counts checked statically. Honor required SDK read results before consuming outputs and close/release the SDK handle when initialization fails.

Resolution: Check required SDK initialization reads and slot capacity before consuming outputs, and close the SDK handle on failed initialization. The permanent SDK-stub suite covers initialization errors and oversized slot counts; the universal production build passed.

2026-09-07 DRV-055–058 implemented in wheel_asi.driver and regenerated: required initialization reads/slot capacity checked before use, failed initialization closes the SDK handle, motion/calibration polling reports read errors without consuming outputs, no-op calibration requests complete, disconnect resets calibration state, and reconnect polls an already-moving wheel. An idle wheel at an unexpected target reports ALERT instead of polling forever. Five permanent public-bus SDK-stub test groups passed, including failed initialization/oversized SDK slot count, polling/calibration errors and recovery, no-op calibration, interrupted calibration/reconnect, normal motion and hot-plug retries. Fresh x86_64/arm64 production build/link passed. Generator unchanged; no hardware test or folder baseline advancement.

### DRV-056 (Closed — fixed)

Related to DRV-048/046: move finalizer ignores EFWGetPosition failure, increments cached current_slot, and compares this fabricated position to the target. Repeated failed reads can falsely finish with OK or continue polling indefinitely if the target is already below the stale value. Reproduced fabricated slot 5/OK with all reads failing. Use a local SDK result, preserve confirmed position on failure, and terminate/report ALERT.

Resolution: Motion polling now checks SDK status and uses confirmed position instead of incrementing cached state. Read errors terminate with ALERT; moving sentinel -1 stays BUSY and an idle wheel at an unexpected position reports ALERT. The permanent SDK-stub suite and universal production build passed.

### DRV-057 (Closed — fixed)

Related to DRV-041/045/046: calibration finalizer initializes pos=0 and ignores EFWGetPosition error. An error without an output write becomes slot 1 and successful Calibration finished, even though physical completion was never confirmed. Reproduced with EFW_ERROR_REMOVED. Check SDK status before interpreting -1/motion or publishing completion; publish ALERT on read failure.

Resolution: Calibration polling checks SDK status before interpreting the position or moving sentinel. Read failure reports ALERT instead of fabricating successful slot 1 completion. The permanent SDK-stub suite and universal production build passed.

### DRV-058 (Closed — fixed)

Incomplete asynchronous calibration completion: START=false is accepted and dispatch sets X_CALIBRATE BUSY, but handler neither updates it nor schedules completion. Disconnect during active calibration also cancels its finalizer without resetting X_CALIBRATE state/switch; reconnect redefines that same BUSY property and further changes are blocked. START=false remaining BUSY across reconnect reproduced; interrupted-calibration variant checked statically. Complete no-op requests and reset interrupted calibration state during lifecycle cleanup.

Resolution: No-op calibration requests complete, disconnect resets the calibration state and switch, and reconnect polls an already-moving wheel. The permanent SDK-stub suite covers no-op and interrupted calibration/reconnect; the universal production build passed.

### DRV-059 (Closed — fixed)

Existing disconnect close guards skip Close for a CCD without a guider, and for standalone wheel/focuser devices whose private camera pointer refers to themselves: their own gp_bits remains 1 until after the guard. These paths can retain the SDK handle/global lock after ordinary disconnect.

Resolution: Corrected the CCD-only and standalone wheel/focuser Close guards while retaining shared CCD/guider `gp_bits` accounting. Removal clears the freed guider pointer before CCD detach. The SDK replacement lifecycle test checks balanced Open/Close and global-lock ownership for both CCD/guider orders and each standalone class. Physical validation remains pending.

### DRV-060 (Closed — fixed)

Wheel calibration and connection initialization wait in unbounded one-second polling loops while SDK position is -1. Moving these bodies unchanged onto a persistent lifecycle/device queue can prevent queued disconnect/removal from progressing. Preserve the operation sequence but provide cancellable delayed completion when migrating these paths.

Resolution: Replaced unbounded initialization and calibration loops with handler/finalizer pairs scheduled through `indigo_execute_handler_in()` on the device queue. SDK commands and the one-second polling interval are preserved. SDK replacement tests cover another device connecting during initialization, calibration cancellation, rejected/resumed shutdown and unplug during initialization. Physical validation remains pending.

### DRV-061 (Closed — fixed)

RA completion cleared WEST twice and left EAST nonzero; completion helpers lacked `_finalizer`, causing the generated OK epilogue to prematurely finish pulses. Corrected names/axis reset, cancellation of replaced completions and transport error propagation; four-direction transport tests pass.

### DRV-062 (Closed — fixed)

Successful ping followed by failed firmware query leaked the serial handle. The handle is now closed on either handshake failure.

Validation: test passes).

### DRV-063 (Closed — fixed)

RTS control errors were ignored and exposures reported success when the output failed. The driver now propagates start/stop and abort errors and allows abort to retry a failed stop even after exposure becomes ALERT. Fake RTS failure/recovery regressions validate these paths.

### DRV-064 (Closed — withdrawn)

The proposed NaN/fractional/range checks duplicated the framework input contract. Removed the checks, associated driver tests and unnecessary value-preservation scaffolding following user review.

Validation: manual selection remains covered.

### DRV-065 (Closed — fixed)

Motion readback overwrote the requested target before comparing it, so any first reply was treated as completion. Replaced blocking loops with bounded queue finalizers, separate measured/target values and validated connection/poll readback.

Validation: measured progress, abort where supported and pending disconnect tests pass.

### DRV-066 (Closed — fixed)

Both move handlers blocked the queue in unbounded read loops and ignored command/read failures. The handlers now use bounded asynchronous polling, queued abort, failure states and cancelable completion callbacks.

Validation: measured progress, abort where supported and pending disconnect tests pass.

### DRV-067 (Closed — fixed)

An already-at-target slew skipped stopping an actively tracking axis, so HOME could wait indefinitely. The axis is now stopped before reading position and calculating the delta. Full SynScan simulator suite passes, with isolated park storage.

### DRV-068 (Closed — fixed)

Start/focus/stop errors were ignored, focus blocked the queue and the configuration switch had no change handler. Added error propagation, cancellable focus completion, busy-request rejection and configuration acceptance. Fake SDK lifecycle and exposure/error tests pass.

### DRV-069 (Closed — fixed)

Timed motion blocked the queue and ignored SDK errors; frequency changes had no handler. Added cancellable timed completion, queued abort, error propagation and frequency acceptance. Fake SDK lifecycle and motion/error tests pass.

### DRV-070 (Closed — fixed)

RA completion cleared WEST twice; missing finalizer convention caused premature OK and SDK failures were ignored. Corrected finalizers, axis reset, replacement cancellation and failure propagation. Four-direction fake SDK tests pass.

### DRV-071 (Closed — fixed)

Move/poll errors were ignored and motion could poll indefinitely; invalid slot/count readback was accepted. Added checked commands/readback, bounded completion and target/readback separation. Fake USB/SDK lifecycle and move-failure tests pass.

### DRV-072 (Closed — fixed)

A GOTO published its target as measured position before motor readback; abort consequently reported the unvisited target. The target is now separate from measured position. An unknown handshake identity is rejected, and generated connection rollback owns the shared handle after malformed initial status.

Validation: measured motion, abort, unknown identity and malformed focuser status tests pass.

### DRV-073 (Closed — fixed)

Disconnect canceled the timer but retained an unfinished target; the generated reconnect timer resumed the old move. Disconnect now freezes the target at the measured position.

Validation: full rotator simulator suite including pending disconnect/reconnect passes.

### DRV-074 (Closed — fixed)

Offset changes never assigned the private motion targets, so only zero/no-op tests passed. The handler now assigns the motion targets, cancels replaced motion and synchronizes abort/disconnect with the timer without duplicating framework input validation. This simulator has no `.driver` source.

Validation: nonzero dual-axis movement, abort, pending disconnect/reconnect and fresh move tests pass.

### DRV-075 (Closed — fixed)

Legacy untracked motion timers survived generated queue teardown; shutter handling blocked the queue for six seconds. Motion and shutter completion now use cancellable queue callbacks, and disconnect freezes unfinished movement.

Validation: full dome simulator suite including pending rotation/shutter disconnect passes.

### DRV-076 (Closed — fixed)

NMEA parsing ran on an uninitialized buffer after read failure, unbounded comma splitting could overrun the token array and short known sentences dereferenced missing fields. Malformed/checksum-rejected sentences disconnected the receiver. Parsing now bounds and validates framing/numbers before dispatch, ignores malformed input and safely queues disconnect after a read failure. Continuous TIME-priority reads also starved settings; reads now requeue at normal priority so pending commands can run.

Validation: malformed input, constellation/fix transitions, coordinate/time mapping, validity and transport recovery tests pass.

### DRV-077 (Closed — fixed)

PPEC training used untracked timers outside the generated queue, allowing post-disconnect transport access. Training polls now run on the owned queue so disconnect cancels them.

Validation: PEC training disconnect/reconnect test passes.

### DRV-078 (Closed — fixed)

Mount disconnect did not cancel the manual-motion timer; guider timers were canceled on connect instead of disconnect. Reversed replacement pulses retained the opposite value. Disconnect now cancels owned timers synchronously and clears movement. Guide replacement clears the replaced axis before copying a request, and duplicate guider connection requests are ignored. This simulator has no `.driver` source.

Validation: full mount simulator suite plus pending manual-motion/guider disconnect and replacement tests pass.

### DRV-079 (Closed — fixed)

Guider RA used AO property macros and omitted the 10 ms duration conversion. Failed writes/reads reused stale acknowledgements, short firmware replies were accepted and failed initial status did not roll back connection. The handlers now use guider properties, require exact reply lengths, clear response storage and propagate command/reset errors. Failed initial status uses generated cleanup.

Validation: fake command/error/rollback/quantization tests, both shared connection orders and PTY AO/guider checks pass.

### DRV-080 (Closed — fixed)

Slot motion slept/polled inside the handler for up to 30 seconds, blocking queued disconnect. Failed WI initialization still connected. Motion now uses cancelable bounded queue polling with measured/target separation and checked initialization, preserving lost-echo retry semantics. Host wheel movement now depends on elapsed time rather than query count and survives PTY disconnect. Input validation and redundant cancellation removed per user review.

Validation: elapsed motion/disconnect/reconnect and fake initialization/lost-response/timeout recovery groups pass.

### DRV-081 (Closed — fixed)

A truncated `r` sensor record passes NULL from `strtok_r` into `indigo_atod` and crashes (fake transport exit 139). The complete record is now parsed into temporary values before publishing; malformed/failed reads retain previous measurements with ALERT.

Validation: all malformed-record, read/write, measurement conversion and reconnect groups pass.

### DRV-082 (Closed — fixed)

Negative read results were treated as success and a partial record still connected. The complete record is now staged and failed reads are rejected before publishing. Protocol simulator pressure is in Pa while the property is labelled hPa; the readback is now converted.

Validation: fake record/failure/unit tests and PTY suite pass.

### DRV-083 (Closed — fixed)

User review identified duplicated framework BUSY/input guards and redundant generator-owned updates. Removed added same-property BUSY blocks, wheel start cancellation/input guards, UPB outlet-name update, dome coordinate update/direction OK assignments and absolute focuser input clamping. Inspected generated property prologues/epilogues without changing the generator.

Validation: simplified wheel/DSUSB/FCUSB and UPB/dome/focuser regression tests pass.

Reviewed the complete current `.driver` diff and corresponding generated handler blocks for duplicate own-property OK/update code, framework BUSY guards and cancellation. This is a focused working-tree audit, not a new whole-folder commit baseline.

Remaining cancellation has a specific purpose:

- DSUSB abort cancels the pending focus-to-exposure transition and old countdown. Removing cancellation was tested: the SDK reopened the shutter one second after abort. The strengthened test also starts a new exposure after abort to detect an old countdown consuming it. The `_finalizer` reference requires one explicit abort OK initialization and one final update; its error branch now assigns only ALERT.
- FCUSB abort cancels a delayed stop that otherwise belongs to the old movement. It must not stop a subsequent move.
- CGUSB/GPUSB guide replacement deliberately accepts replacement while an axis is active; it cancels the old handler/completion. GPUSB now preserves the previous stop on failed replacement.
- Astromechanics/USBv3 share one motion finalizer between separate POSITION and STEPS request paths. Their individual property macros do not provide a shared guard before both handlers can be queued, so a replacement cancels the prior scheduled poll. These are not the wheel's single-property start case.
- Hand-written mount/polar-align simulator timer cancellation is tied to abort, replacement or disconnect. Generated disconnect already cancels queued handlers; no extra cancel-all was added to those `.driver` disconnect blocks.

Own-property updates retained outside generated completion are asynchronous start/completion or early-return/custom-message cases. Updates to another property (for example AO reset clearing axis alerts) are not generated by that handler and remain explicit.

### DRV-084 (Closed — fixed)

Replacement canceled the old stop before the SDK accepted the new pulse; failure could leave a relay on. Zero-duration handling also called SDK stop twice (regression observed two calls instead of one). The relay mask is now committed and the old completion canceled only after SDK success. Zero completion is published without a second SDK write.

Validation: fake replacement failure and single-write zero-stop regressions pass.

### DRV-085 (Open)

The manufacturer's CloudWatcher protocol v1.3 specifies `RH = raw * 1.25 - 6` for `!h` replies. The driver instead uses `(raw * 1.7572) / 100 - 6`. The protocol simulator returns `!h 40`: the bus publishes **-5.29712%** instead of **44%**. Reproduced by `test_aux_cloudwatcher_simulator`, scenario `humidity_conversion`. This also feeds the driver's dewpoint and humidity warning calculations. Production code intentionally remains unchanged.

### DRV-086 (Open)

In `aag_command()`, a 15-second timeout with zero bytes leaves `index == 0`; the failure branch writes `response[index - 1]`. `AUX_TEST_FILTER=timeout ./build/integration/test_aux_cloudwatcher_simulator_asan` reproduces a one-byte stack-buffer-underflow through a silent real PTY. The ordinary build can reject the connection while silently corrupting memory, so its passing timeout scenario is not evidence of memory safety. Production code intentionally remains unchanged.

### DRV-087 (Open)

`lunatico_command()` reads up to `max` UDP bytes and then writes `response[index] = '\0'`. A 100-byte datagram fills the 100-byte array used during connection and writes one byte beyond it. `AUX_TEST_FILTER=oversized ./build/integration/test_aux_dragonfly_simulator_asan` reproduces the stack-buffer-overflow at `shared/dragonfly_shared.c:121` using a separate loopback UDP simulator. The non-instrumented executable survives the same case; ASan is necessary to expose the corruption. Production code intentionally remains unchanged.

### DRV-088 (Closed — fixed)

`parse()` accepts a checksummed sentence without validating its field count. The GPS RMC handler immediately reads `tokens[1]`/`tokens[9]` (line 204), and the weather XDR handler reads `tokens[2]` and other missing fields (line 345). The standalone simulator sends truncated `GPRMC` or `PXDR` sentences with correctly computed checksums. Both `short_gps` and `short_weather` crash in the normal build; the ASan build confirms NULL reads in the conversion calls. This concerns malformed device replies, not framework property input validation.

Fixed in the 2026-09-09 working tree, generated version `0x0300000A`: `aux_mgbox/indigo_aux_mgbox.driver:74` bounds tokenization and checksum parsing; `:148` checks required fields before conversion and `:308` bounds per-instance framing. Both original short-message reproducers and expanded malformed/split-frame recovery scenarios pass, including six selected driver-instrumented ASan parser scenarios. The final ordinary suite passes all 32 scenarios. See `aux_mgbox/REFACTOR.md`; folder review baseline is unchanged.

### DRV-089 (Closed — fixed)

`mgbox_open()` tests the pointer returned by `indigo_uni_open_serial_with_speed()` with `>= 0` (line 431), and `data_refresh_callback()` uses the same comparison in its loop (line 195). A NULL handle therefore still takes the success/reader path. Last close first clears the handle and then waits for that reader (lines 473–475), which does not exit on a cleared pointer. The real-PTY `normal` and `gps_readings` cases pass their data assertions but fail disconnect/shutdown, leaving the driver attached. The nonexistent-port case fails to reach a disconnected ALERT state within the bounded wait. Source and compiled-driver tests agreed at the baseline.

Fixed in the 2026-09-09 working tree, generated version `0x0300000A`: `aux_mgbox/indigo_aux_mgbox.driver:351` implements transactional pointer-handle acquisition and `:394` closes the acquired handle; generated connection handlers own shared references and rollback. Bounded reads and callbacks on the master queue replace the independent reader. Invalid port, silent identification, three reconnect cycles, both shared orders, secondary rejection with the primary active, pulse/reboot disconnect and two independent instances pass. ASan also passes both pending-operation disconnect scenarios and instance isolation. The final ordinary suite passes all 32 scenarios; no hardware/TCP validation or folder-wide review is claimed.

### DRV-090 (Open)

The vendor's `USB2ST4_Conv.h` documents error returns for ON and OFF operations. The driver logs failed ON calls but still marks the axis BUSY and schedules successful completion; timer callbacks ignore OFF errors entirely. `start_failure` and `stop_failure` in `test_guider_asi_sdk` inject `USB2ST4_ERROR_GENERAL_ERROR` at the documented SDK boundary. Neither reaches ALERT. In the OFF case the fake SDK retains the asserted relay, while the driver reports completion and clears its private relay state. Confirmed with the unchanged x86_64 driver under Rosetta; no driver fix applied.

### DRV-091 (Open)

The RA replacement branch cancels the old timer, then enables WEST without first disabling EAST. The SDK documents independent per-direction ON/OFF calls, not implicit interlocking. The `reversal` scenario starts EAST for 500 ms and replaces it with WEST for 100 ms: the fake SDK records both bits (12) instead of WEST alone (8). The DEC branch follows the same pattern; the runtime reproducer currently exercises RA. No driver fix applied.

### DRV-092 (Open)

An all-zero RA request cancels the previous completion timer at line 259 and enters neither positive-duration branch. No OFF call or replacement completion is scheduled, and the old private relay flag leaves the property BUSY. The `zero_stop` scenario starts EAST for 500 ms, sends zero and observes the output still asserted after the bounded two-second wait. This is cancellation of a valid active operation, not generic framework numeric validation. No driver fix applied.

### DRV-093 (Open)

`last_action` is set to INIT before hot-plug registration succeeds. After a simulated registration error, a second INIT returns OK through the repeated-action shortcut without registering a callback or discovering the present device. `registration_failure_retry` deliberately retries INIT directly, without an intervening SHUTDOWN that would conceal the problem. No driver fix applied.

### DRV-094 (Open)

Hot-plug arrival schedules `process_plug_event` on an untracked 0.5-second timer. SHUTDOWN unregisters the USB callback and removes the current device array but does not cancel or join already scheduled work. `shutdown_pending_arrival` performs INIT followed immediately by SHUTDOWN; after 800 ms the previously scheduled callback has attached one device. The bus cannot stop because that device remains attached. The test uses the real framework timer, not a replacement scheduler. No driver fix applied.

### DRV-095 (Open — protocol confirmation needed)

The standalone PTY simulator exposes an independently sourced compatibility discrepancy: upstream PHD2 `src/scope_GC_USBST4.cpp` at commit `c406cf2b2de51cbb7e3ed76165c9accf33edad2d`, lines 108–117, sends `:Mg0`, `:Mg1`, `:Mg2`, `:Mg3` for NORTH/SOUTH/EAST/WEST; INDIGO sends `:Mgn`, `:Mgs`, `:Mge`, `:Mgw`. The PHD2-dialect test rejects the letter command even though INDIGO later reports successful timer completion. The explicitly labelled INDIGO-dialect scenario passes all four directions over real I/O.

This is **not yet proof of a hardware bug**: the manufacturer's historical product/protocol page was unavailable, and firmware may accept both encodings. The simulator does not silently treat INDIGO as the protocol authority. Manufacturer documentation or a device trace is needed to resolve dialect support; no hardware test or driver change was made.

### DRV-096 (Open)

`focuser_lunatico/shared/lunatico_shared.c:336`, included by `rotator_lunatico/indigo_rotator_lunatico.c:35`: `lunatico_command()` allows `index == max` and then writes `response[index] = '\0'`. A 100-byte reply overflows the 100-byte response in `lunatico_check_port()` (line 375). The serial loop admits all 100 bytes; the UDP branch also reads `LUNATICO_CMD_LEN` without reserving terminator space or respecting a smaller caller capacity. The unchanged `oversized` scenario passes without instrumentation but fails with a one-byte stack-buffer-overflow when both the test and production driver translation unit are compiled with AddressSanitizer. The ASan stack identifies `lunatico_command`, `lunatico_open` and `handle_rotator_connect_property`; the affected stack object is `response` at line 375. Reserve one byte for termination and reject truncated/oversized replies. No production source change applied.

Reproduction from `indigo_test/`: compile `integration/test_rotator_lunatico_simulator.c` together with `../indigo_drivers/rotator_lunatico/indigo_rotator_lunatico.c`, using the normal include/library paths, `-fsanitize=address -fno-omit-frame-pointer -g -O1`, and link `libindigo` instead of the uninstrumented driver archive; run with `AUX_TEST_FILTER=oversized`. Reproduced on macOS arm64, 2026-09-09.

### DRV-097 (Open)

`focuser_lunatico/shared/lunatico_shared.c:1543–1547`: after a rejected `!step goto`, the rotator handler sets `ROTATOR_POSITION` to ALERT internally but never publishes that state. It unconditionally schedules `rotator_timer_callback()`, whose idle readback replaces ALERT with OK at line 1365. Clients see BUSY followed by OK although the device rejected the requested move. The supplied `goto_failure` case fails waiting for ALERT in both serial and isolated UDP runs, with the driver logging `lunatico_goto_position(...) failed`. Publish the command failure and avoid treating a subsequent idle poll as successful completion of the rejected move. No production source change applied.

### DRV-098 (Closed — fixed — 2026-09-09)

Baseline `focuser_lacerta/indigo_focuser_lacerta.c:88` writes a terminator after allowing a full-size reply. The new `overlong_identity` simulator case reproduces a stack-buffer-overflow in `lacerta_command` with the unchanged production driver instrumented by ASan. Handwritten transport version `0x02000002` passes the reproducer; repeat against final generated source before closing.

### DRV-099 (Closed — fixed — 2026-09-09)

Baseline `focuser_lacerta/indigo_focuser_lacerta.c:59` ignores write failure and can return true after the expected reply never arrived. Connection accepts unknown/truncated identity; silent identification does not reach bounded disconnected ALERT. `unknown_identity`, `short_identity` and `silent_identity` reproduce these failures. Initial numeric readback and setting/poll error handling also require validation during migration.

### DRV-100 (Closed — fixed — 2026-09-09)

Baseline `focuser_lacerta/indigo_focuser_lacerta.c:153` changes motion state only when the measured position changes. `noop` reproduces a request for the current position stuck BUSY. Abort also leaves the old target and does not finalize motion state; add abort/restart and stalled/read-failure regressions as part of the scoped migration.


### Lunatico simulator validation — 2026-09-09

Scoped execution of the existing working-tree `integration/test_rotator_lunatico_simulator.c` at `eed97b62e6a07c28d6629ca0508770ceb89dde08`. The driver archive was up to date; serial and UDP test build targets succeeded. No driver/shared-source changes exist between the recorded folder baseline and this commit in the inspected Lunatico paths. This is a focused validation, not a complete folder review.

- Serial: 6/11 scenarios passed; `exp_rotator`, `third_rotator`, `exp_focuser`, `third_powerbox` and `goto_failure` failed.
- Isolated loopback UDP: 6/11 scenarios passed, with the same five failing scenario names. `exp_focuser` terminated with SIGSEGV in this run; its exact crash cause remains unresolved.
- Both normal runs passed `main_rotator`, `abort`, `read_failure`, `wrong_model`, `silent` and `oversized`. ASan reveals the hidden memory error in `oversized` (DRV-096).
- A temporary diagnostic copy inserted a 500 ms delay before connection to allow asynchronous attachment: `exp_focuser` then passed, while `exp_rotator` connected but timed out waiting for the 20-degree readback. This is timing evidence only, not a permanent test fix or proof of the SIGSEGV cause. A separate ASan `exp_focuser` run failed at startup without reproducing that crash. The diagnostic copy and binaries were removed after validation.
- The first UDP attempt was blocked by sandbox bind restrictions; an initial permitted run overlapped the serial suite and encountered the driver's global lock. Only the subsequent isolated UDP run is used for the counts above.
- Secondary-device failures require care: test `start()` at lines 21–29 requests asynchronous device creation/reconfiguration and immediately enumerates/connects the target. `connect_serial_device()` only sets the simulator endpoint when `DEVICE_PORT` is already defined. The observed failures attempt `auto://` (one serial run also selected an automatically enumerated system port), not the simulator endpoint. These failures do not establish that the secondary motion or powerbox protocols are broken. Driver configuration returns OK before asynchronous attachment completes (`lunatico_shared.c:1008–1027`); test readiness and concurrent configuration need further investigation. No physical-device acceptance was performed.

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
| `6efc2c7ac` | `a5282c84d` | 2026-09-01 | Verification pass over today's race-fix commits (DRV-022 through DRV-029); confirmed no deadlocks introduced. Surfaced `DRV-030` as a pre-existing NULL-handle crash in `ccd_touptek` disconnect path, independent of these commits. |
| `017ba602857378e4aed489c065c76eacae15924c` | `1e82d6187` + working tree | 2026-09-01 | Deep focused review of `mount_ioptron` generated driver, generator source, simulator, and integration coverage; recorded `DRV-031` as a generator-template lifecycle bug exposed by `mount_ioptron`, plus `DRV-032` and `DRV-033` as iOptron-specific findings. Did not advance the folder baseline because the rest of `indigo_drivers` was not reviewed. |
| `017ba602857378e4aed489c065c76eacae15924c` | HEAD + working tree | 2026-09-01 | Deep focused review of `mount_lx200`, including the incremental tracking-mode diff, hand-written shared connection lifecycle, new host-side simulator, and LX200 integration coverage; recorded `DRV-034` and `DRV-035`. Did not advance the folder baseline because the rest of `indigo_drivers` was not reviewed. |
| `017ba602857378e4aed489c065c76eacae15924c` | HEAD + working tree | 2026-09-01 | Deep focused review of `mount_synscan`, including the incremental driver/protocol diff, refactored host-side simulator, mount/guider integration coverage, UDP endpoint parsing, and pulse-guiding lifecycle; recorded `DRV-036` and `DRV-037`. Did not advance the folder baseline because the rest of `indigo_drivers` was not reviewed. |
| `017ba602857378e4aed489c065c76eacae15924c` | `a35e4ce49483ac2f3cba7b69c20372c2f93a63f3` + working tree | 2026-09-07 | Focused `focuser_asi` definition/generated-output review and subsequent fixes; recorded and closed `DRV-039` and `DRV-040`. Generator reproduction, universal build and SDK-stub checks recorded in the finding summaries. Folder baseline unchanged. |
| Not recorded | `109506ce0` + working tree | 2026-09-07 | Focused `focuser_asi` lifecycle, motion, settings and SDK-error review, followed by fixes and six public-bus SDK-stub regression groups; recorded and closed `DRV-041` through `DRV-050`. The exact starting revision was not recorded. Folder baseline unchanged. |
| Not recorded | working tree | 2026-09-07 | Focused `wheel_asi` SDK-ID reservation review; recorded and closed `DRV-051`. Failed-attach/capacity retry regression and universal build passed. Exact revision bounds were not recorded; folder baseline unchanged. |
| Not recorded | working tree | 2026-09-07 | Focused bundled SDK/HIDAPI inventory and static run-loop analysis; recorded `DRV-052` through `DRV-054`, which remain open pending reproduction. Exact revision bounds were not recorded; folder baseline unchanged. |
| Not recorded | working tree | 2026-09-07 | Focused `wheel_asi` comparison with the focuser findings, followed by initialization, motion and calibration fixes; recorded and closed `DRV-055` through `DRV-058`. Five permanent SDK-stub groups and universal build passed. Exact revision bounds were not recorded; folder baseline unchanged. |
| `30e667c8cd0b57c0aa0feb285ae79a651ce3dd2c` | working tree | 2026-09-08 | Focused ToupTek migration inventory and step 2 resolution of `DRV-059` and `DRV-060`: shared SDK-close ownership and cancellable wheel initialization/calibration. SDK replacement validation recorded in the finding summaries; physical validation remains pending. Folder baseline unchanged. |
| HEAD / pre-build working tree | working tree after `make all` | 2026-09-08 | Focused generator-impact review: build regenerated 54 additional C files; 51 contain only generated `free()` to `indigo_safe_free()` substitutions, while `aux_dsusb`, `focuser_fcusb`, and `guider_gpusb` also receive the approved libusb lifecycle fixes already generated and tested in `ccd_sx`. Compared every build-induced diff and verified custom annotated blocks unchanged; no header/main or unrelated source changes were produced by the build. Full macOS x86_64/arm64 build passed; warnings concern vendor SDK deployment targets. No additional regression found in this delta. Hardware tests were not repeated. Folder baseline unchanged. |
| HEAD | working tree | 2026-09-08 | Follow-up on the SDK/HID omission documented as TOOLS-009: all 13 generated hot-plug drivers now contain the shared queue drain before detach (4 libusb, 7 SDK, 2 HID), including ccd_dsi and both HID wheels. Custom driver blocks preserved; this focused pass does not cover hand-written hot-plug implementations or advance the folder baseline. |
| HEAD | working tree | 2026-09-08 | Focused ToupTek follow-up: aligned hand-written shutdown with the generated SDK lifecycle, including shared queue task mutex, rejection before deregistration, and drain before detach. Preserved SDK-id enumeration and camera/guider/wheel/focuser cleanup. Fake SDK lifecycle, pending-event and hot-plug rollback/removal scenarios passed. Folder baseline unchanged. |
| `84298256404b3aee1028d29ce152233ffd8afe2e` | working tree | 2026-09-09 | Scoped non-CCD generated-driver and simulator review during hardware-free coverage work; recorded `DRV-061` through `DRV-082`. Fixes and available fake SDK/USB/transport or simulator validation are recorded per finding. `DRV-064` was withdrawn after user review. This covers the named drivers and paths only, not the complete non-CCD inventory; folder baseline unchanged. |
| `84298256404b3aee1028d29ce152233ffd8afe2e` | working tree | 2026-09-09 | Follow-up over the changed driver definitions and corresponding generated handlers for redundant framework validation, BUSY guards, cancellation and property updates; recorded and closed `DRV-083` and `DRV-084`. Relevant regressions and the complete project build passed. Folder baseline unchanged. |

| `e29626f7da814e4b756c496c6b7bc6ab98328baa` | working tree | 2026-09-09 | Scoped protocol-documentation and runtime review of `aux_cloudwatcher`, `aux_dragonfly` and `aux_mgbox` only. Added standalone PTY/UDP tests; recorded open `DRV-085`–`DRV-089`, including driver-instrumented ASan reproducers. Production drivers unchanged; folder baseline not advanced. |

| `f8713fa21` | working tree | 2026-09-09 | Scoped guider pass: unchanged ASI driver with vendor-header-based fake SDK on x86_64/Rosetta; CG-USB-ST4 standalone PTY and explicit PHD2/INDIGO dialect profiles; existing GPUSB fake SDK regression suite. Recorded `DRV-090`–`DRV-094` as reproduced bugs and `DRV-095` as an unresolved protocol discrepancy. No production source changes; folder baseline unchanged. |

| `017ba602857378e4aed489c065c76eacae15924c` | `eed97b62e6a07c28d6629ca0508770ceb89dde08` + working-tree tests | 2026-09-09 | Scoped rotator_lunatico serial/UDP simulator execution and shared-source failure analysis. Recorded DRV-096 and DRV-097, secondary attachment/test timing failures, and an unresolved SIGSEGV. Driver sources unchanged; folder baseline not advanced. |

LACERTA migration disposition (scoped; folder review baseline unchanged): DRV-098 is fixed by bounded framing in `focuser_lacerta/indigo_focuser_lacerta.driver:43`; final overlong identity/poll cases pass with the production driver instrumented by ASan. DRV-099 is fixed by validated bounded transactions, transactional identity open (`:97`) and explicit post-open initialization rollback (`:233`); all nine identity/initialization rejection cases pass, including descriptor-count checks and retry for one-shot faults. DRV-100 is fixed by explicit no-op completion (`:185`), queue-owned motion finalization and abort (`:329`); no-op, relative, overlap, abort/restart, failed stop and stalled/poll-failure scenarios pass. Final suite: 43/43; targeted ASan: 19/19. Earlier paragraphs preserve reproduction history. No full-folder review or baseline advancement is implied.


### DRV-101 (Closed — fixed — 2026-09-09)

Against baseline `2509190db7f4697427ea463a27b0f683c637f09e`, the new `init_status` test reproduced a successful connection with invalid FI initialization; `poll_badflag` reproduced acceptance of moving=2. The authoritative DSL validates exact framing, field widths/digits, coordinate range and 0/1 flags, requires successful status initialization and explicitly closes on post-open failure. Both reproductions pass after the generated transition, with descriptor rollback/retry coverage. Additional malformed and lost-reply cases are documented in test CHANGES.md. This is a scoped disposition; folder review baseline is unchanged.

### DRV-102 (Closed — fixed — 2026-09-09)

Baseline property allocation used literal `ZERO_SYNC` despite its X_FOCUSER_ZERO_SYNC C macro. The DSL now declares wire name `X_FOCUSER_ZERO_SYNC` with the same SYNC item, connected lifetime and reset-after-request semantics. README and PROPERTIES document the client-visible rename. Normal/model capability tests assert the new property and absence of the old name.


### DRV-103 (Closed — fixed — 2026-09-09)

Baseline efa_command reads packet-provided count+1 bytes into a 16-byte response buffer. `init_overlong` with the unchanged driver instrumented by ASan reproduces a 201-byte stack-buffer-overflow through indigo_read; `init_checksum` reproduces connection accepting a bad checksum. The DSL now bounds frames, verifies checksum/source/destination/command, skips only exact echoes and checks payload/ACK semantics. Both reproducers and initial/poll variants pass after migration, including ASan. No generator implementation or folder review baseline was changed.

### DRV-104 (Closed — fixed — 2026-09-09)

The old temperature path reads fixed offsets without validating reply length or the documented 7F7F absent-sensor marker. The old simulator emits 00 50 01, which becomes 1280.0625 C under that parser. The PDF itself disagrees between its three-byte prose and two-byte example; the driver now explicitly accepts address + big-endian signed sixteenths, plus a documented legacy two-byte little-endian compatibility form. It rejects NC/impossible temperature and retains the last valid value. Positive/negative/zero/NC/recovery fixtures cover both shapes. Physical firmware formats still need hardware validation; no claim that a PTY resolves the document's ambiguity.


### DRV-105 (Closed — fixed — 2026-09-09)

The original focuser and auxiliary connection handlers unconditionally reopen and overwrite one shared handle before consulting its count, and both attach paths initialize the same mutex. Generated ownership now opens on the first successful logical connection, increments only successful opens, rolls back failed initialization, and closes after the last logical disconnect. Shared queue cancellation retains the other device's pending work. Simulator tests exercise slave-first connection, descriptor count, failed slave initialization while the focuser remains usable, either disconnect order and independent instances. Folder review baseline is unchanged.

### DRV-106 (Closed — fixed — 2026-09-09)

The supplied one-page command table requires OK_PRDG identity, matching command echoes, H=0, Z=Z:1, and four D booleans. Legacy prefix matching accepts OK_OTHER (reproduced with original C/header); power/USB/park handlers report success despite rejected replies, and outlet 2 is decoded as 2. The DSL validates complete delimited responses, numeric fields and acknowledgements, uses actual B speed readback, and confirms power/USB state after writes. Fault tests cover malformed/partial/overlong/silent replies, ACK/partial writes and recovery. Park completes only at zero/idle, with abort and stalled-motion handling. PTY tests do not validate physical encoder travel or firmware reboot timing.

## Focused Player One abort latency investigation (2026-09-09)

Target: `94e1ef2bb`; inspected Player One acquisition/abort code and Imager Agent abort routing against the recorded baseline. No subtree review-marker advancement.

- The generated `ccd_change_property()` uses ordinary `INDIGO_COPY_VALUES_PROCESS_CHANGE` for `CCD_ABORT_EXPOSURE` (`ccd_playerone/indigo_ccd_playerone.c:1720`). This schedules NORMAL priority, while acquisition finalizers use `indigo_execute_handler_in()` (TIME priority). Higher abort priority would reduce waiting behind ready tasks, but cannot preempt a running callback.
- The macro sets abort BUSY before notifying clients and enqueueing the handler. `acquisition_finalizer()` checks that state on entry (`ccd_playerone/indigo_ccd_playerone.driver:338`), so priority alone does not explain four or five additional delivered streaming frames after the driver has accepted an abort.
- An in-flight finalizer does not recheck abort after SDK read or before synchronous `indigo_process_image()` (`.driver:360`, `.driver:384`). Hardware runs in continuous SDK mode even for single INDIGO exposures (`.driver:409`). Sensor acquisition can therefore continue during readout/image processing until `POAStopExposure()` is reached. This is a plausible latency contributor, not a measured explanation of the user report.
- Existing fake-SDK tests filtered by `abort` passed: long exposure/guiding abort and reacquire, deterministic frame-before-abort versus abort-before-frame ordering, and setup/wait/removal abort coverage. The ordering test permits one delivered frame when abort arrives during readout; it does not reproduce a network client, short-exposure series controller or physical SDK timing. The reporting user's acquisition mode/client are unknown.

Next diagnosis should distinguish request receipt, abort BUSY publication, finalizer entry/exit, SDK stop, and terminal property publication. A generator priority change would need a separately approved concrete proposal; no generator or production driver changes were made for this investigation.

## Working-tree abort change self-review (2026-09-10)

Scope: requested review of this task's uncommitted changes relative to `94e1ef2bb`, including all 25 `.driver` inputs and their generated C. This does not advance the subtree baseline. Build/runtime scenario mapping belongs to `indigo_test/CHANGES.md`.

- DRV-107, P1, Closed (fixed): URGENT abort overtook NORMAL operation starts, allowing motion/exposure to start after abort completed. All 27 affected abort branches now cancel the associated pending starts. Example: `ccd_sx/indigo_ccd_sx.driver`, `CCD_ABORT_EXPOSURE.on_change`.
- DRV-108, P2, Closed (fixed): the initial cancellation prologues bypassed existing abort-switch/BUSY guards. Cancellation now follows each existing abort condition; DSUSB and RTS barrier tests explicitly verify false abort preservation. Example: `aux_dsusb/indigo_aux_dsusb.driver`, `CCD_ABORT_EXPOSURE.on_change`.
- DRV-109, P1, Closed (fixed): a canceled start never schedules its completion callback, so abort paths relying solely on that callback left BUSY properties unresolved. Explicit settlement or a scheduled existing completion path covers pending acquisition, shutter, relative dome motion, calibration, park/home and focuser motion. Pending motion is settled before a potentially failing stop where no finalizer exists. Example: `dome_skyroof/indigo_dome_skyroof.driver`, `DOME_ABORT_MOTION.on_change`.

Per-driver cancellation/settlement audit:

| Drivers | Checked path |
| --- | --- |
| aux_dsusb, aux_rts, mount_synscan AUX | Abort switch guard; pending exposure, shutter timers/finalizers and exposure terminal publication. |
| ccd_dsi, ccd_sx | Exposure BUSY guard includes queued starts; existing hardware stop and CCD cleanup remain paired. |
| ccd_playerone | Exposure/streaming guard; active acquisition finish versus cleanup before SDK start. |
| aux_upb, aux_upb3, focuser_dmfc, focuser_fc3 | Existing switch guard and periodic hardware motion readback; stop success terminal updates preserved. |
| focuser_efa, focuser_prodigy, focuser_ioptron, focuser_lacerta | Existing guard; pending motion settled even if stop fails; EFA calibration and Prodigy park cannot remain BUSY without a finalizer. |
| focuser_asi, rotator_asi | Existing unconditional stop; pending start cancellation and motion-status completion; ASI focuser pending properties settled before stop failure. |
| focuser_fcusb | Existing unconditional stop and motion-finalizer cancellation; steps terminal update preserved. |
| focuser_primaluce focuser/rotator | Pending properties settled independently of a not-yet-scheduled movement finalizer. |
| focuser_usbv3 | Moving flag controls hardware stop, not start cancellation; queued motion terminated without altering idle completed properties. |
| dome_simulator | Guard includes horizontal, relative and park BUSY; existing timer cleanup invoked after canceling pending starts/timer. |
| dome_skyroof | Shutter BUSY guard; existing finalizer scheduled even when start never ran; explicit abort publication because the finalizer reference suppresses generator epilogue. |
| mount_ioptron | Existing abort switch guard; pending park/home properties and switches settled; existing motion/coordinate stop updates retained. |
| mount_nexstaraux | Existing switch guard; pending coordinates and park settled independently of the slewing flag; manual axes settled before a failed stop. |
| mount_pmc8 | Existing unconditional stop; pending park flag/property and manual-axis states settled. |
| mount_synscan mount | Existing unconditional stop; pending park/home and coordinate/manual-axis properties settled before state-light refresh. |
| rotator_simulator | Existing abort/BUSY guard; pending position canceled and target reset to current position. |

Generated-diff normalization confirmed that all other generated function bodies are unchanged: differences are abort dispatch, abort bodies and start-handler forward declarations. Regeneration is reproducible. DSL and generated C versions are unchanged by user instruction. No new locks, properties or refactoring notices were introduced. Existing running SDK/transport calls remain non-preemptible; the original physical Player One latency report is not reproduced by these hardware-free checks.

Validation result: 23/24 complete integration suites passed; the remaining focuser_ioptron suite passed 39/40 in concurrent runs and its unchanged, non-abort `overlap` scenario passed in isolation. This timing-sensitive test result is retained as a validation limitation, not declared fixed. New queue-barrier abort tests, generator/timer tests, final targeted reruns and all affected builds passed; details are in `indigo_test/CHANGES.md`.

## Physical Player One abort follow-up (2026-09-10)

DRV-110, P2, Closed (fixed): `ccd_playerone/indigo_ccd_playerone.driver:1116` (`CCD_ABORT_EXPOSURE.on_change` else branch; generated C lines 1160–1164) sets ALERT and resets the switch without an update when no exposure/stream is BUSY. The on_change block references acquisition_finalizer, suppressing the generated epilogue. The public request's BUSY therefore persists at clients. Reproduced on physical Mars-C II between 0.1 s exposures and in 9/20 final Imager Agent abort trials; the agent itself completes. Fixed in the `.driver` source and regenerated C: the idle branch resets the switch and explicitly publishes ALERT; active abort retains its existing cleanup publication. The regression test failed before the fix and passes afterward, including false/true idle requests, completed-exposure gaps, no extra SDK stop, and subsequent acquisition. The four/five-extra-frame report was not reproduced in the 60 final trials; see TESTING.md. Folder baseline unchanged.

DRV-110 verification: all five abort-filtered SDK cases passed. Physical idle abort before and after acquisition passed; PREVIEW and indefinite STREAMING each passed 20 trials with camera terminal publication in all 40 and no frames after terminal completion. One streaming trial delivered one frame before completion. Driver version is unchanged; no locks were added. Folder review baseline remains unchanged.


## Mount Agent test findings — 2026-09-10

Scoped test-driven inspection of `agent_mount/indigo_agent_mount.c` at
`f13fdc6965bdc306d4250248e0ef8ed19527d991`, driver version `0x03000016`.
The production driver was not changed. The folder baseline above is unchanged;
this is not a review of the rest of `indigo_drivers`.

`indigo_test/integration/test_agent_mount.c` exercises the production agent through
public bus requests with controlled mount, dome, rotator, GPS, joystick and related
agent peers. It also drives the unchanged LX200 worker through a scripted portable
I/O boundary. Real timers, handler queues and filter translation are retained.
The listener fake blocks until closed, matching `indigo_uni_open_tcp_server_socket`.
No network sockets, server process or physical devices are required.

The strict suite deliberately keeps correct-behavior assertions for open findings:
failures are not marked expected or converted into passes. Sixteen failing scenarios
map to DRV-134–DRV-147 above. Shutdown watchdog failures count as test failures and
terminate only the isolated child. Use `make -C indigo_test test-agent-mount`, or pass
a case-name substring to `indigo_test/build/integration/test_agent_mount`.

Coverage includes successful, failed, aborted and timed-out operations; legacy and
modern state reporting; coupled device motion and abort fan-out; configuration,
reselection, related agents, limits, joystick routing and the LX200 command set.
The detailed scenario-to-test map, measurements and validation limitations live in
`indigo_test/CHANGES.md`. Existing fixes DRV-016/017/018/020/021 have passing checks
for disabled joystick/default modes, abort fan-out and mount/rotator deselection or
slaving-alert propagation. The separate legacy dome reselection issue is DRV-134.

Validation observation: the 51-case sanitizer pass had one additional watchdog
termination in `negative fits`; an immediate isolated rerun reached the expected
DRV-135 assertion without a sanitizer diagnostic. No separate root cause was
established. Treat this as an unresolved timing observation, not a confirmed memory
finding or proof that all concurrency interleavings are safe.

### DRV-134 fix validation — 2026-09-10

Mount Agent version increased from `0x03000016` to `0x03000017`. Deselection
clears the previous dome's state and capability flags; a subsequent DOME_STATE
definition selects modern reporting, otherwise legacy properties remain active.
The two original regressions failed before the fix (0/2), and the expanded
reselection checks pass afterward (3/3). They verify cleared lights/capabilities,
restored OPEN state on reselection and subsequent close/park/unpark/open operations.
Four additional cases pass: `legacy success`, `modern success` (11 operations
each), `selection orders` and `slaving`. Compilation covered macOS arm64/x86_64;
execution was hardware-free on arm64. The original 54-case coverage/results above
remain the pre-fix baseline; the entire suite was not rerun for this scoped fix.
DRV-135–DRV-147 remain open, including individual-property deletion (DRV-146).

### DRV-135 fix validation — 2026-09-10

The imager's negative OBJCTDEC minute expression now multiplies by 60 before
integer truncation, matching the positive-coordinate path. The expanded
`negative fits` regression passes four exact fixtures: -12.5, -12.125, -0.125
and -90 degrees. `related agents` passes its positive FITS, coordinate-forwarding
and related-process checks on an isolated retry. The initial pre-fix regression
run and first related-agent run hit the child watchdog during setup, consistent
with the unresolved timing observation above; neither produced a new numerical
assertion result. Compilation succeeded for macOS arm64/x86_64; execution was on
arm64. Version remains `0x03000016` at the user's request, with the version bump
deferred until the final fix. DRV-136 is separate and remains open.

### DRV-136 fix validation — 2026-09-10

Guider OBJCTDEC and imager SITELAT/SITELONG now derive a separate minus prefix
from the original coordinate and format absolute integer degrees. This preserves
negative subdegree values without changing positive values or zero. The two
expanded regressions check -0.5, -0.125, -12.125, 0, 0.125 and 12.125 degrees for
each affected field, including seconds and forwarded numeric guider declination.
The expanded site regression failed before the change. Afterward, all three
FITS cases and all three related-agent cases pass (6/6), including DRV-135's
imager regression. Built macOS arm64/x86_64 and executed on arm64.

The initial guider runs hit the previously noted setup watchdog. The test peer
selection helper now waits for a same-priority queue marker after the filter's
reverse-relation/enumeration task, before emitting coordinate updates. The filter
publishes list OK before that task finishes; the new wait serializes this test
setup without changing production queue or filter behavior. Both affected test
groups pass with this synchronization. This is not a general concurrency audit.
Version remains `0x03000016`, with the bump deferred until the last fix as requested.

### DRV-137 fix validation — 2026-09-10

The ACK branch now directly sends one P byte using the portable I/O API. Colon
command parsing and replies are unchanged. `lx200 ack` failed before the fix;
afterward it passes single ACK, consecutive ACKs, ACK/GVP/ACK with separators,
and an ACK following an unknown command, with exact expected response bytes.
`LX200 protocol` and `lx200 input matrix` also pass (3/3 targeted cases). Built
macOS arm64/x86_64 and executed hardware-free on arm64 through the scripted
transport boundary. Version remains `0x03000016` as requested. DRV-138 onward
remain separate open findings.

### DRV-138 fix validation — 2026-09-10

Both Sd parsing branches now distinguish an explicit -00 prefix from +00 instead
of classifying every zero-degree value as negative. The expanded regression failed
before the fix and passes afterward for 12 inputs: signed subdegree values in full
and minute-only formats, seconds-only offsets, nonzero degrees and zero. It checks
both the success replies and published target declination at epoch 2000.
`LX200 protocol` and `lx200 input matrix` also pass (3/3 targeted cases). Built
macOS arm64/x86_64 and executed on arm64 using the scripted transport boundary.
Version remains `0x03000016` as requested; framing and numeric validation findings
DRV-139/140 remain separate and open.

### Shared sexagesimal formatter follow-up — 2026-09-10

At the user's request, the manual Mount Agent FITS conversions used by the DRV-135/DRV-136 fixes have been replaced with `indigo_dtos_r()`, the new caller-buffer implementation behind `indigo_dtos()`. The original API retains its rotating static buffers for compatibility; all production callers now use independent caller storage, including the LX200 worker's existing output buffer. The formatter already rounds according to its supported format, including carry at 60; FITS values now round to seconds rather than truncate. Negative subdegree signs and FITS quoting are preserved.

Validation: all 150 unit cases, nine relevant Mount Agent cases, three iOptron simulator cases and four LX200 simulator cases passed. All nine affected drivers/agents and the framework built for macOS arm64/x86_64; execution was on arm64. Coverage details are in `indigo_test/CHANGES.md`. This is a scoped implementation follow-up, not a new incremental review or an advance of the review baseline. Existing open findings and deferred driver versions are unchanged.

### DRV-139 fix validation — 2026-09-10

The LX200 worker now records complete # termination before dispatch. EOF/read failure inside a command and an oversized frame terminate the worker through its existing socket cleanup. Up to 128 payload bytes plus the terminating # fit in the 129-byte buffer; longer frames are not truncated into executable commands, and their suffix is not parsed as a new command on the same connection.

Both regressions failed before the fix. `lx200 truncated command` now checks six partial commands under EOF and read error (12 fixtures), no replies or target updates, exactly one connection close, preservation of the previously accepted RA/DEC, and a complete command followed by a partial setter. `lx200 command length` checks 127/128/129-byte payload boundaries, an oversized setter, discarded trailing commands, unchanged requested coordinates and a subsequent healthy connection. These two cases plus ACK, signed zero, full LX200 protocol and input matrix passed (6/6). The agent and test executable built for macOS arm64/x86_64; execution was on arm64 without hardware or sockets. Driver version remains 0x03000016 as requested. DRV-140 coordinate syntax/range validation remains separate and open.

### DRV-140 fix validation — 2026-09-10

Replaced permissive Sr/Sd sscanf parsing with a shared bounded-field coordinate parser. It requires two-digit fields, canonical separators, an explicit declination sign, RA below 24 hours, DEC within ±90 degrees, and minutes/seconds below 60. At either pole every subordinate component must be zero. The entire payload must match, so malformed suffixes, whitespace, signed subordinate fields, non-finite values, exponent/hex forms and oversized integer fields are rejected without modifying the previous requested coordinate. Invalid commands return 0 and the connection remains usable.

The bundled `mount_lx200/Meade-2010.10.pdf` specifies SrHH:MM.T / SrHH:MM:SS and SdsDD*MM / SdsDD*MM:SS. Short RA decimal minutes are now decoded instead of silently discarded; existing whole-minute RA and fractional-second support are retained. `lx200 invalid coordinates` covers 48 malformed/range fixtures with unchanged-target checks and subsequent recovery. `lx200 coordinate boundaries` covers six valid pairs including zero, 23:59:59, ±90, signed subdegree values and fractional minutes/seconds. Both tests failed before the fix and now pass. Together with ACK, signed-zero, framing/length, full protocol and input-matrix regressions, 8/8 cases passed. Production agent and tests built for macOS arm64/x86_64, with arm64 execution using scripted I/O and no hardware/sockets. Version remains 0x03000016 per the user's instruction.

### DRV-141 fix validation — 2026-09-10

Added `indigo_uni_open_tcp_server_socket_with_callback()` to carry caller context into the existing listener-ready callback on both POSIX and Windows paths. The original public function remains a wrapper with its original callback signature. Mount Agent uses a per-invocation context whose lifetime covers the blocking listener call, publishes STARTED/OK only after listen succeeds, and publishes STOPPED/ALERT if opening returns before readiness. BUSY publication now precedes scheduling, preventing the listener from completing before the initial request update.

The scripted listener now mirrors the ready-callback contract. `lx200 bind failure` holds each open attempt before completion, verifies BUSY and no open listener, and exercises failure → successful retry/query/normal stop → failure. All nine selected LX200 cases and all 150 unit cases passed. Framework, agent and tests built for macOS arm64/x86_64; execution was on arm64 with hardware-free scripted I/O. The new low-level callback wiring was inspected on both platform branches; real socket failure modes and Windows execution were not exercised. Driver version remains 0x03000016 per the user's instruction. DRV-142/143 stop/shutdown findings remain separate and open.

### DRV-142 fix validation — 2026-09-10

The NULL-handle branch of `stop_lx200_server()` now selects STOPPED, sets OK and publishes the result. The active-listener path still closes the listener and lets its existing completion publish the final state.

Expanded `lx200 idle stop` failed before the fix and passes afterward. It checks two initial idle stops, stop after failed startup, a successful restart/query/normal stop and two subsequent idle stops. Switch values, terminal states and close counts verify that an idle stop does not close any socket and a running listener closes exactly once. All ten selected LX200 regressions passed, including cleanup; production agent and tests built for macOS arm64/x86_64, with arm64 execution using scripted I/O and no hardware/sockets. Driver version remains 0x03000016 per the user's instruction. DRV-143 shutdown ordering remains a separate open finding.

### DRV-143 fix validation — 2026-09-10

Moved `stop_lx200_server()` before `indigo_cancel_all_timers()` in device detach. The listening socket is therefore closed before waiting for the timer that runs the blocking listener, while property/private-data release remains after the timer wait. Configuration is saved after the listener has completed.

Expanded `shutdown server` hit SIGALRM (status 14) with the pre-fix ordering. After the change, three active-listener start/query/shutdown/reinitialization cycles pass under the five-second watchdog. The transport double counts listener returns, and the test checks that the callback has finished before driver shutdown returns, the listening socket closes exactly once, and reinitialized state is STOPPED. Thirteen selected cases passed: all eleven LX200 cases including shutdown plus two persistence cases. Agent and test executable built for macOS arm64/x86_64; tests ran on arm64 using the scripted blocking listener without actual sockets/hardware. This validates shutdown with an established listener; shutdown during an in-progress socket open and live worker-connection lifetime are not covered here. Driver version remains 0x03000016 as requested. Active mount-motion shutdown (DRV-144) remains separate and open.

### DRV-144 fix validation — 2026-09-10

Before detaching the internal client, driver shutdown now sends AGENT_ABORT_PROCESS through the existing public handler when AGENT_START_PROCESS is BUSY, then calls `indigo_cancel_pending_handlers()` while the client is still alive. The queue API removes pending tasks and waits for the running handler to return. This preserves abort forwarding and incoming peer updates during process termination, and prevents freeing the client while an active handler still uses it. Idle shutdown does not send an abort.

The expanded `shutdown active` regression hit SIGALRM (status 14) before the fix. It now passes nine five-second-watchdog cycles covering SLEW, SYNC, PARK, UNPARK, HOME, DOME_PARK, DOME_UNPARK, DOME_OPEN and DOME_CLOSE with mount/dome/rotator selected and coupling enabled. Each cycle checks one abort request per selected device and clean reinitialization; a final idle shutdown checks unchanged abort counts. Twelve selected cases passed, including active/server shutdown, coupled and legacy/modern aborts, legacy/modern successes, listener lifecycle, LX200 protocol and persistence. Production agent and tests built for macOS arm64/x86_64; execution was on arm64 with scripted peers and no hardware/sockets. This verifies abort requests and software teardown, not physical motion cessation. Version remains 0x03000016 as requested.

### DRV-145 fix validation — 2026-09-10

After copying an accepted START request, the agent checks whether any operation is selected. If none is selected it publishes OK and returns before the BUSY/start dispatch. The existing BUSY guards run before this check, so an all-false request cannot clear an active operation. This is handling of the valid at-most-one empty selection, not duplicate framework validation.

Expanded `all false start` failed before the fix and passes afterward. It covers idle empty requests, empty requests after a missing-device error, repeated full all-false vectors with peers selected, exactly one property update and unchanged peer command counts, subsequent successful syncs, and an empty request during a held slew followed by normal completion. Eleven selected cases passed: this regression, missing devices/capabilities, legacy/modern success and abort matrices, active/server shutdown and both persistence cases. Production agent and test executable built for macOS arm64/x86_64; hardware-free tests ran on arm64. Version remains 0x03000016 as requested.

### DRV-145 scope extension — other agents (2026-09-10)

The same START pattern exists in Imager and Guider. With both required devices selected, an all-false request leaves BUSY without any scheduled completion; with devices missing it produces an unrelated ALERT instead. Both now use the same empty-selection completion as Mount Agent, before BUSY and before Imager pause/abort resets.

ASTAP (`agent_astap/indigo_agent_astap.c:683`) and Astrometry (`agent_astrometry/indigo_agent_astrometry.c:1016`) delegate to the common platesolver. Its START handler scheduled `start_process()` for every non-reset request, and that function called `start_exposure()` even when no operation was selected. The common handler now completes an empty request with OK. Its redundant pre-guard property copy was removed so a rejected BUSY request cannot overwrite the active selection. Test Agent sets BUSY only inside selected-test branches and does not share this stuck-BUSY pattern; the remaining agent implementations do not expose this START handler.

New Imager/Guider `empty start` tests cover repeated empty vectors without devices, with camera only and with the second device selected; exactly one OK update and no exposure requests; subsequent valid single preview; and a rejected empty request during continuous preview followed by abort. New `platesolver empty start` uses a minimal device/client fixture with the real shared platesolver/filter APIs, verifies no-op behavior without and with a related Imager Agent, and confirms that an explicit SOLVE still dispatches and reports the missing-source error. It requires no solver executable/index or sockets. All three regressions fail against their original handlers and pass with the fixes (the original platesolver object was linked into a separate temporary executable for baseline validation).

Eleven selected cases passed across Mount, Imager and Guider suites, including the platesolver fixture, existing capture/preview/abort/reset tests and BUSY guards. Framework, Imager, Guider, ASTAP and Astrometry builds passed for macOS arm64/x86_64; tests executed on arm64 using the CCD simulator. Versions remain deferred per user instruction. This is a scoped extension of DRV-145, not an advance of the folder review baseline.

### DRV-146 fix validation — 2026-09-10

Mount Agent now derives feature flags from the selected devices' cached property definitions on definition/deletion notifications. Deleted source entries are excluded even while the filter is still announcing deletion of their agent clones. Ordinary status updates no longer set capability flags. The common calculation preserves dome SLEW while either DOME_ON_COORDINATES_SET/GOTO or DOME_HORIZONTAL_COORDINATES remains and publishes feature changes immediately, including restoration.

Expanded `stale capability` checks mount HOME/PARK/TRACK and dome PARK/OPEN removal, unsupported-operation ALERT with no peer command, restored flags and successful operations after redefinition; mount SLEW/SYNC loss and restoration; both deletion/restoration orders for the dome's two slew sources; and isolation from an unselected peer's deletion. Twelve selected integration cases passed, including missing devices/capabilities, legacy/modern success, three dome reselection cases, process deselection, legacy dome slew, active shutdown and empty start. Agent and test executable built for macOS arm64/x86_64; hardware-free execution was on arm64. Version remains 0x03000016 per user instruction; test artifacts removed. Review baseline unchanged.

### DRV-147 fix validation — 2026-09-10

The common slew/sync path now waits for required unpark completion before sending any coordinate-mode or coordinate commands to the mount, coupled dome or rotator. It skips unpark when already unparked or when parking is unsupported, waits for both required devices, and rejects ALERT, abort, deselection, removed parking capability or the existing-style bounded 180,000 × 1 ms wait limit. Failure clears SLEW/SYNC and finishes START with ALERT; an abort request is acknowledged. Coordinate geometry is calculated after waiting. Sync retains its existing mount/rotator scope.

Modern and legacy `unpark failure` matrices cover immediate rejected slew and sync, delayed mount/dome failure, abort, dome disconnect, accelerated timeout and successful recovery with the mount completing before the dome. Assertions forbid mode/coordinate commands on failure and while the dome remains pending, including rotator commands. Existing operation-timeout and coupled-abort tests now explicitly complete unpark recovery before testing subsequent slew behavior. The new success fixture waits for the rotator movement request before publishing completion, avoiding an early-completion race.

The full Mount Agent executable passed 59/59 cases after the final changes; both new matrix cases also passed five repeated runs. Production agent/test builds passed for macOS arm64/x86_64; execution used scripted hardware-free peers on arm64. Test artifacts removed. Version remains 0x03000016 per user instruction; review baseline unchanged.

### Deferred driver versions finalized — 2026-09-10

After completion of DRV-134–DRV-147, the user authorized the deferred version increments, including the caller-buffer formatting changes and shared platesolver fix. Earlier validation notes describe the versions at the time of each fix.

| Driver | Previous | Updated |
| --- | --- | --- |
| `agent_mount` | `0x03000016` | `0x03000017` |
| `agent_imager` | `0x03000039` | `0x0300003A` |
| `agent_guider` | `0x0300002D` | `0x0300002E` |
| `agent_scripting` | `0x0300000A` | `0x0300000B` |
| `agent_astap` | `0x0200000A` | `0x0200000B` |
| `agent_astrometry` | `0x02000015` | `0x02000016` |
| `mount_asi` | `0x0300001C` | `0x0300001D` |
| `mount_ioptron` | `0x0300002F` | `0x03000030` |
| `mount_lx200` | `0x03000032` | `0x03000033` |
| `mount_mxhd` | `0x0001` | `0x0002` |
| `mount_rainbow` | `0x0200000E` | `0x0200000F` |

iOptron version was updated in the `.driver` source (47 → 48) and regenerated; the generated diff changes only DRIVER_VERSION. All eleven driver builds passed for macOS arm64/x86_64.
The regenerated iOptron driver passed all three serial simulator cases on arm64; test artifacts were removed.

## Configuration Agent — full behavioral test audit (2026-09-10)

User-requested scope: `agent_config` and a new hardware-free test suite. Production source was inspected in full for test planning, together with `git diff 017ba602857378e4aed489c065c76eacae15924c..HEAD -- indigo_drivers/agent_config`; the observed HEAD was `a321d776c459f9a07295d2eca611daac5b7ffcd3`. This is a scoped audit, not a completed review of all drivers; the folder baseline is unchanged. Production agent code and version `0x03000007` are unchanged. Standard AGENT_CONFIG names were checked against `indigo_names.h` and the documented Configuration agent section; no property was added or removed.

`indigo_test/integration/test_agent_config.c` exercises the real agent and framework through the public bus, using local peer properties, isolated filesystem paths, bounded waits, and a separate process/watchdog for every case. It never starts a server, opens a network socket, loads a hardware driver, or modifies HOME. The test-only agent build substitutes the configuration-directory/HOME lookup, waits, client-detach observation and a serialization-failure seam. All successful serialization and all XML restoration use the production implementations.

Findings DRV-148–DRV-168 are recorded in the main findings table above. Every finding has a failing behavioral regression; failures remain real failures (exit status 1), not expected-failure passes.

### Validation and limits

- Universal macOS arm64/x86_64 compilation succeeded. Execution on arm64: **30/55 cases passed, 25 failed**, corresponding to **21 distinct open findings** above. No fixes were folded into this test-only task. `make -C indigo_test test-agent-config` and the normal integration target therefore fail until those regressions are fixed. Run an individual case by passing its name to `indigo_test/build/integration/test_agent_config`.
- AddressSanitizer run: the same results, with no AddressSanitizer memory-error report in the instrumented agent/framework/harness. The existing static dependency libraries were not rebuilt with the sanitizer. The autosave reentrancy case terminates by watchdog, so successful teardown is not claimed for that case.
- Native clang source coverage: **20/20 functions (100%), 720/742 lines (97.04%), 1204/1260 regions (95.56%), 328/375 branches (87.47%)**. Assertions/macros contribute branches. This is broad behavioral coverage, not a claim of 100% line/branch coverage. Remaining unexecuted lines are allocation/base-attach failure guards, defensive empty-token exits after `strtok_r`, and release of still-pending restore entries at the start of a load. Those were not forced by modifying production logic or passing invalid callback pointers.
- With the original agent wait durations (`INDIGO_CONFIG_REALTIME=1`), save/load roundtrip and missing-profile failure/recovery pass; restore-BUSY timeout reproduces DRV-153. Accelerated waits otherwise preserve the original loop counts and state decisions.
- Positive coverage includes schema/defaults/permissions, filtered enumeration, INFO and idempotent lifecycle, rediscovery, setup save/load/restart, local/remote discovery, profile/filter compaction, all filter classes, NONE and multiple related selections, XML escaping, overwrite, whitespace normalization, normal removal/recovery, autosave options, unload policy for known items, repeated load, delayed profile appearance, BUSY-to-OK, rejected selection/related agents, queue BUSY guard, capacity reuse, bounded long-list truncation (DRV-003), and idle/active shutdown. Active shutdown passed the held-worker fixture without post-detach peer commands; this does not prove all arbitrary shutdown interleavings.
- The peer router models the documented synchronous local server/property contract. Real dynamic-library load/unload, external clients, real device CONFIG persistence, Windows/Linux execution, OOM behavior and exhaustive thread interleavings are not covered. No hardware is required for the agent's own tested behavior. Detailed scenario mapping and reproducible commands are in `indigo_test/CHANGES.md`.

Test build artifacts were removed with `make -C indigo_test test-clean` after verification.

### DRV-148 fix validation — 2026-09-10

Missing files and input containing no restorable configuration now finish with LOAD and LAST ALERT instead of reporting success. Tests passed: `missing_file`, `malformed_file`, `save_roundtrip`, `missing_profile`. Universal macOS build; hardware-free arm64 execution.

### DRV-149 fix validation — 2026-09-10

Discovery now requires the final .saved suffix and removes only that suffix, preserving embedded .saved text in configuration names. Tests passed: `scan_suffix`, `startup_scan`, `save_roundtrip`. Universal macOS build; hardware-free arm64 execution.

### DRV-150 fix validation — 2026-09-10

Removal now uses the portable configuration directory and path separator, checks path truncation, and retains normal removal and last-configuration behavior. Tests passed: `alternate_folder_remove`, `remove_roundtrip`, `save_roundtrip`. Universal macOS build; hardware-free arm64 execution.

### DRV-151 fix validation — 2026-09-10

Discovery and removal now share the framework port suffix policy, expose logical configuration names, filter other nondefault ports, and retain the unsuffixed ephemeral-port behavior. Tests passed: `port_namespace`, `nondefault_port_load`, `remove_roundtrip`, `scan_suffix`, `save_roundtrip`. Universal macOS build; hardware-free arm64 execution.

### DRV-152 fix validation — 2026-09-10

The early deselection-timeout exit now publishes LAST ALERT as well as LOAD ALERT, so the visible last-configuration state is finalized. Tests passed: `deselection_timeout`, `missing_profile`, `save_roundtrip`. Universal macOS build; hardware-free arm64 execution.

### DRV-153 fix validation — 2026-09-10

A restore operation that exhausts its BUSY polling budget now fails explicitly; successful delayed completion still passes. Tests passed: `restore_busy_timeout`, `busy_then_ok`, `selection_failure`, `save_roundtrip`. Universal macOS build; hardware-free arm64 execution.

### DRV-154 fix validation — 2026-09-10

Profile restoration now waits for confirmed selection and successful device status, including rejecting ALERT when the requested profile was already selected. Internal status storage follows profile insertion/deletion and is released on detach; recovery is covered. Tests passed: `profile_rejection`, `delayed_profile`, `discovery_compaction`, `save_roundtrip`, `missing_profile`. Universal macOS build; hardware-free arm64 execution.

### DRV-155 fix validation — 2026-09-10

Driver restoration now checks the server terminal status and actual switch values, reports rejection or timeout, and supports successful retry after rejection. Tests passed: `driver_rejection`, `driver_unload_policy`, `save_roundtrip`, `repeated_load`. Universal macOS build; hardware-free arm64 execution.

### DRV-156 fix validation — 2026-09-10

A missing matching agent is now an incomplete restore condition and ends with ALERT when the bounded wait expires, rather than being treated as already restored. Tests passed: `absent_agent`, `restore_busy_timeout`, `busy_then_ok`, `save_roundtrip`. Universal macOS build; hardware-free arm64 execution.

### DRV-157 fix validation — 2026-09-10

Whole-device deletion of the tracked server now clears its driver snapshot and routing name, while unrelated device deletion does not clear the server. Restoration does not broadcast driver commands to an empty device name. Tests passed: `server_disappearance`, `deletion`, `discovery`, `save_roundtrip`. Universal macOS build; hardware-free arm64 execution.

### DRV-158 fix validation — 2026-09-10

Profile mirroring clears the previous selection before reading the current vector, so an all-false update no longer retains a stale profile name. Tests passed: `profile_no_selection`, `discovery`, `profile_rejection`, `save_roundtrip`. Universal macOS build; hardware-free arm64 execution.

### DRV-159 fix validation — 2026-09-10

The restore queue now holds all sixteen agents plus driver/profile records, reuses completed slots, reports overflow, and releases pending entries at detach. Tests cover sixteen-agent roundtrip, 64 sequential direct restores, overflow and recovery. Tests passed: `capacity_restore`, `direct_restore_capacity`, `restore_queue_overflow`, `repeated_load`, `shutdown_idle_reinitialize`. Universal macOS build; hardware-free arm64 execution.

### DRV-160 fix validation — 2026-09-10

Autosave snapshots each agent selection under data_mutex, then releases the mutex before synchronous CONFIG SAVE requests. A reentrant profile update now completes without deadlock. Tests passed: `autosave_reentrant`, `autosave`, `save_roundtrip`. Universal macOS build; hardware-free arm64 execution.

### DRV-161 fix validation — 2026-09-10

SAVE and REMOVE now reject mutation while LOAD is BUSY. Barrier-based tests confirm no file or load-selection changes, the original load completes successfully, and ordinary saving/removal still work. Tests passed: `concurrent_save`, `concurrent_remove`, `busy_guard`, `remove_roundtrip`, `save_roundtrip`. Universal macOS build; hardware-free arm64 execution.
