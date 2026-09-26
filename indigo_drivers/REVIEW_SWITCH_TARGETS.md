# Switch target adoption review (branch `refactoring_targets`)

This file belongs to the `refactoring_targets` branch and to the internal switch target change only. It
is not part of the incremental review in `REVIEW.md` and is not listed in the root `REVIEW.md` index.

## Background

A status poll, timer, finalizer, reader thread or another handler can overwrite the items of a switch
property after a client request was copied into it on the bus thread and before the queued change
handler runs. A handler that reads `sw.value` then sends the device the overwritten state instead of
the request, and usually reports OK. The framework change on this branch gives switch items an
internal `sw.target`:

- `indigo_property_copy_values()` writes it together with the value, including the one-of-many reset;
  `indigo_set_switch()` and direct `sw.value` writes do not touch it; it is never sent over the protocol.
- A handler reads the request with `indigo_get_switch_target()`, calls `indigo_apply_switch_targets()`
  when the device accepted it, and on failure sets the switch to the state the device reports before
  publishing ALERT.
- Only requests guarded by BUSY (`INDIGO_COPY_VALUES_PROCESS_CHANGE`) may rely on it, not the
  `_ANYTIME` / `accept_while_busy` variants.
- Reference adoption: `mount_ioptron` `MOUNT_TRACKING` (3.0.0.59), see `mount_ioptron/REFACTOR.md`
  and `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md` ("Switch Item Target").

## Audit

Read-only audit of 2026-09-26 on `refactoring_targets` at `90c0922`, all device drivers under
`indigo_drivers/`, `indigo_linux_drivers/` and `indigo_mac_drivers/`. Agents and the off-limits
drivers (`ccd_apogee`, `ccd_sbig`, `mount_asi`, `mount_mxhd`, `system_ascol`) were not audited.
Line numbers are in the driver's `.driver` file unless a file is named; `.c` means the generated
driver. Line numbers drift as drivers change; re-read the code before fixing.

Severity:

- **High:** the background writer has no BUSY guard, so the window is the whole time the handler is
  queued.
- **Medium:** the writer checks `P->state != INDIGO_BUSY_STATE`, so the window is only between that
  check and the write, as in `mount_ioptron` `MOUNT_TRACKING` before the fix.
- **Low:** only the displayed value or state is wrong for a while; no request is lost.

Status values: `Open`, `Fixed (<version>, <date>)`, `Not a target case` (needs another fix, see
section B), `Won't fix` with a reason.

Polls, finalizers and change handlers of one device run one at a time on the device queue
(`indigo_libs/indigo_driver.c:1542`); aux+gps and aux+focuser devices of one driver share it.
`INDIGO_TASK_PRIORITY_TIME` tasks (polls, finalizers) run ahead of normal handlers, which widens the
gap between the copy and the handler.

## A. Switch properties the target fixes

### A1. High: background writer without a BUSY guard

| ID | Driver | Property | Async handler | Background writer | Handler reads | Note | Status |
| --- | --- | --- | --- | --- | --- | --- | --- |
| TGT-001 | mount_simulator | MOUNT_TRACKING | on_change 445 | `position_handler` (slew finalizer): 123 turns tracking ON at the end of a slew, 115 OFF on home | 448 | A pending OFF is lost at the end of a slew. | Fixed (3.0.0.22, 2026-09-26): the handler reads the request with `indigo_get_switch_target()` and applies it with `indigo_apply_switch_targets()`; regression test `mount_tracking_request_survives_slew_end`. |
| TGT-002 | mount_temma | MOUNT_SIDE_OF_PIER (RW, 387) | on_change 609 | `temma_update_position` 210, from on_timer 336, goto finalizer 251, park finalizer 276 | 610 | The request is overwritten with the current side, no `PT` is sent and the handler reports OK. | Superseded (3.0.0.21, 2026-09-27): MOUNT_SIDE_OF_PIER is read-only again, so no request can be overwritten; the 3.0.0.19 handler and its test are removed. |
| TGT-003 | mount_synscan | MOUNT_TRACKING | on_change 1716 | `mount_equatorial_coordinates_finalizer` 1495; `synscan_clear_tracking_state` 786 from park finalizer 1523, home finalizer 1548, `mount_motion_failed` 1476 | 1718 | 787 also forces the state to OK over BUSY (see TGT-B05). | Fixed (3.0.0.12, 2026-09-26): the handler reads the request with `indigo_get_switch_target()`, applies it with `indigo_apply_switch_targets()` and on failure shows the RA axis state with ALERT; the finalizers still write the value but leave the state to a pending request; regression test `synscan_mount_tracking_request_survives_home_finalizer`. |
| TGT-004 | mount_synscan | MOUNT_PEC_TRAINING | on_change 1928 | `synscan_ppec_training_timer` (1 s, armed at 1933), write 1038 | 1929, 1932 | A START queued while training ends becomes STOP. | Fixed (3.0.0.12, 2026-09-26): the handler sends the target and applies it, on failure shows the training state the mount last reported with ALERT, and the timer no longer publishes over a pending request; regression test `synscan_mount_pec_training_request_survives_training_end` (simulator option `--ppec-training-seconds`). |
| TGT-005 | mount_pmc8 | MOUNT_TRACKING | on_change 777 | `mount_goto_complete` 508 from `mount_goto_finalizer` 551 | `pmc8_set_tracking_rate` 298 | 510/512 also force the state (TGT-B05). | Fixed (3.0.0.15, 2026-09-27): the handler sends the request read with `indigo_get_switch_target()`, applies it with `indigo_apply_switch_targets()` and on failure shows the drive state the controller reports (`ESGx`) with ALERT; `mount_goto_complete` still starts tracking and writes the value but leaves state and publication to a pending request; regression test `pmc8_mount_tracking_request_survives_goto_end`. |
| TGT-006 | mount_nexstaraux | MOUNT_TRACKING | on_change 604 | `mount_slew_finalizer` (every 0.1 s during GOTO), write 532/537 | 605, 608 | Tracking changes are accepted during a GOTO. 533/538 force the state (TGT-B05). | Fixed (3.0.0.22, 2026-09-26): the handler reads the request with `indigo_get_switch_target()`, applies it with `indigo_apply_switch_targets()` and on failure shows the rate the mount was last given with ALERT; regression test `tracking_request_survives_slew_end`. |
| TGT-007 | mount_nexstar | TRACKING_MODE | on_change 835 | `nexstar_update_position` (on_timer 774), write 354/356 | 837, 548/550 | The guard at 346 is on MOUNT_TRACKING, not TRACKING_MODE. A client AUTO is replaced by the detected EQ/AA while detection runs. | Fixed (3.0.0.46, 2026-09-26): the poll leaves a BUSY TRACKING_MODE alone and the handler sends the targets, applies them with `indigo_apply_switch_targets()` and on failure shows the mode the mount reports with ALERT; regression test `nexstar_tracking_mode_request_survives_detection`. |
| TGT-008 | dome_nexdome3 | DOME_SHUTTER | on_change 834 | `handle_shutter_status` (reader thread 196 -> queued `nexdome3_process_messages` 243), write 494/500/506/516 | 835 | Triggered by the unsolicited `:SES` when the shutter stops. | Open |
| TGT-009 | dome_nexdome3 | DOME_PARK | on_change 851 | `handle_rotator_status`, write 450 (park_detection branch) | 852 | Only right after connect (park_detection set at 721): a pending UNPARK becomes PARKED. | Open |
| TGT-010 | focuser_ioptron | FOCUSER_REVERSE_MOTION | on_change 271 (.c:463) | `ioptron_publish()` 129 from on_timer 223 (1 s) and `motion_finalizer` 152; also the ABORT (295) and X_FOCUSER_ZERO_SYNC (316) handlers | 272 | The request is lost silently; 130 also sets the state OK over BUSY (TGT-B05). | Fixed (3.0.0.10, 2026-09-27): `ioptron_publish()` leaves a BUSY FOCUSER_REVERSE_MOTION alone and the handler sends the request read with `indigo_get_switch_target()`, applies it with `indigo_apply_switch_targets()` and on failure shows the direction the focuser last reported with ALERT; regression tests `reverse_request_survives_poll`, `reverse_request_survives_poll_failure`. |
| TGT-011 | focuser_steeldrive2 | X_USE_PID (aux) | on_change 844 (.c:1397) | `steeldrive2_read_aux(device, false)` 346 from aux on_timer 798 (1 s); also the AUX_HEATER_OUTLET handler 815 | 845 | Old value sent, read back as matching, reported OK. 799 forces the state (TGT-B05). | Fixed (3.0.0.18, 2026-09-27): `steeldrive2_read_aux()` (aux poll and AUX_HEATER_OUTLET handler) leaves a BUSY X_USE_PID alone and the handler sends the request read with `indigo_get_switch_target()`, applies it with `indigo_apply_switch_targets()` when the device reports it and otherwise shows the setting the device last reported with ALERT; regression tests `aux_pid_request_survives_poll`, `aux_pid_request_survives_heater_change`. |
| TGT-012 | focuser_steeldrive2 | X_USE_AUTO_DEW (aux) | on_change 827 (.c:1394) | `steeldrive2_read_aux` 347, same paths as TGT-011 | 828 | Same as TGT-011. | Fixed (3.0.0.18, 2026-09-27): same fix as TGT-011 for X_USE_AUTO_DEW; regression test `aux_auto_dew_request_survives_poll`. |
| TGT-013 | focuser_prodigy | AUX_POWER_OUTLET (aux) | on_change 496 (.c:853) | `prodigy_publish_ports()` 430-431 from `reboot_finalizer` 445 | 497 | Narrow: only a change queued while `reboot_finalizer` succeeds. | Fixed (3.0.0.8, 2026-09-27): `prodigy_publish_ports()` leaves a BUSY AUX_POWER_OUTLET alone and the handler sends the request read with `indigo_get_switch_target()`, applies it with `indigo_apply_switch_targets()` when the device reports it and otherwise shows the outlets the device last reported with ALERT; regression test `power_request_survives_reboot`. |
| TGT-014 | focuser_prodigy | AUX_USB_PORT (aux) | on_change 519 (.c:856) | `prodigy_publish_ports()` 432-433 from `reboot_finalizer` 445 | 520 | Same as TGT-013. | Fixed (3.0.0.8, 2026-09-27): same fix as TGT-013 for AUX_USB_PORT; regression test `usb_request_survives_reboot`. |
| TGT-015 | aux_wcv4ec | AUX_COVER | on_change 392 (.c:545) | on_timer 152-155 and 160-163 (no guard), 168-170 (guarded) | 397 | Reachable when `operation_running` is set but AUX_COVER is not BUSY, i.e. after AUX_DETECT_OPEN_CLOSE (279). The timer also sets OK over BUSY and clears `operation_running`. | Fixed (3.0.0.10, 2026-09-27): the timer still ends a detection but leaves value and state of an AUX_COVER request that is BUSY before its handler started a move (`operation_start_time` 0) alone, and the handler sends the request read with `indigo_get_switch_target()`, applies it with `indigo_apply_switch_targets()` and on failure or refusal shows the side the last status frame reports with ALERT; regression test `cover_request_survives_detection_end`. |
| TGT-016 | aux_dragonfly | AUX_GPIO_OUTLETS | on_change 156 (.c:355) | `relay_pulse_finalizer` `shared/dragonfly_shared.c:260`, write 266, publish 271 | `dragonfly_set_outlets` shared 292-303 | Narrow: only relays whose pulse just ended, set to false. | Won't fix (2026-09-27): no request is lost. The finalizer writes false only into relays whose pulse has elapsed but is still shown ON, and the handler treats such a relay as running (`relay_pulse_until` still set), so an ON request copied in that window sends nothing with or without the write, like the same request a moment earlier; after the pulse length was set to 0 the write is what keeps the stale ON from switching the relay on. The target cannot replace the write: in an any-of-many request the items the client did not send keep their previous target, so the handler cannot tell a new ON from the ON of the finished pulse. A temporary experiment on 3.0.0.8 (queue held by a gate handler past the pulse end, then a request for OUTLET_2 only) showed that with the finalizer skipping a BUSY property and the handler reading targets relay 1 is pulsed again (switched on for good after its length was set to 0), while the unchanged driver sends only `rlset 0 1 1`. The finalizer's update during BUSY shows the device state and does not change the property state. |
| TGT-017 | dome_dragonfly | AUX_GPIO_OUTLETS | on_change 488 | same shared `relay_pulse_finalizer` (`aux_dragonfly/shared/dragonfly_shared.c:266`) | shared 292/295/302 | Same code as TGT-016; fix both together. | Won't fix (2026-09-27): the dome relay device has the same `AUX_GPIO_OUTLETS` on_change and includes the same `aux_dragonfly/shared/dragonfly_shared.c`, so the TGT-016 analysis applies unchanged; no code change, dome_dragonfly version unchanged. |
| TGT-018 | aux_asiair | AUX_GPIO_OUTLETS | on_change 275 (.c:486) | `relay_pulse_finalizer` 108 (`sw.value = false` when a pulse ends) | `asiair_set_outlets` 134/137/144/145 | Practically harmless: the finalizer switches the hardware off in the same step and the handler compares with a fresh hardware read. | Open |
| TGT-019 | aux_rpio | AUX_GPIO_OUTLETS | on_change 318 (.c:595) | `relay_pulse_finalizer` 123 | `rpio_set_outlets` 149/152/159/160 | Same as TGT-018. | Open |

### A2. Medium: background writer with a BUSY guard

| ID | Driver | Property | Async handler | Background writer (guard -> write) | Handler reads | Note | Status |
| --- | --- | --- | --- | --- | --- | --- | --- |
| TGT-020 | mount_lx200 | MOUNT_TRACKING | on_change 3241 | `meade_update_mount_state` (on_timer 2559), 2486 -> 2488/2491 | 3242, 3243 | Same pattern as the ioptron reference. | Open |
| TGT-021 | mount_lx200 | MOUNT_PARK | on_change 3007 | `meade_update_mount_state`, else branch of 2496 -> 2507/2510 | 3008, 3016, 3031 | A lost PARK/UNPARK is silently refused by the park_allowed check. | Open |
| TGT-022 | mount_lx200 | MOUNT_HOME | on_change 3072 | `meade_update_mount_state`, else branch of 2514 -> 2527/2529/2533 | 3073 | The poll can clear HOME and the handler does nothing. | Open |
| TGT-023 | mount_lx200 | MOUNT_TRACK_RATE | on_change 3254 | onstep `meade_update_onstep_state` 1787 -> 1815; nyx `meade_update_nyx_state` 2024 -> 2036 | `meade_set_tracking_rate` 894-944 | OnStep: the guard is checked before the `:GT#` round trip (1804) and not again before the write. | Open |
| TGT-024 | mount_lx200 | ONSTEP_AUTO_MERIDIAN_FLIP | on_change 2905 | `meade_update_onstep_state` 1818 -> 1821 | 2907 | | Open |
| TGT-025 | mount_lx200 | AUX_POWER_OUTLET (aux) | on_change 3721 | `onstep_aux_update` (aux on_timer 3557) 3485, re-checked 3490 -> 3497 | 3724 | Very small window. | Open |
| TGT-026 | mount_ioptron | MOUNT_PARK | on_change 1512 | `ioptron_update_mount_state`, else branch of 1426 -> 1435/1438 | 1513 | | Open |
| TGT-027 | mount_ioptron | MOUNT_HOME | on_change 1538 | `ioptron_update_mount_state`, else branch of 1442 -> 1448/1451 | 1540, 1545 | HOME overwritten to AWAY: the handler reports OK and never homes. | Open |
| TGT-028 | mount_nexstar | MOUNT_TRACKING | on_change 819 | `nexstar_update_position` 346 (also requires OFF) -> 361 | `nexstar_set_tracking` 547 | The window includes the `tc_get_tracking_mode` round trip (347). Only OFF -> ON can be overwritten. | Fixed (3.0.0.46, 2026-09-26): the poll checks BUSY again after the round trip and the handler reads the request with `indigo_get_switch_target()`, applies it with `indigo_apply_switch_targets()` and on failure shows the tracking the mount reports with ALERT; regression test `nexstar_tracking_request_survives_detection_round_trip`. |
| TGT-029 | dome_baader | DOME_SHUTTER | on_change 662 | `dome_status_poll` 374 -> 387 (via 285/287) | 666 | The state is re-read at 385. | Open |
| TGT-030 | dome_baader | DOME_FLAP | on_change 682 | `dome_status_poll` 404 -> 417 (via 293-297) | 686 | Same re-read at 415. | Open |
| TGT-031 | dome_beaver | DOME_HOME | on_change 676 | `dome_status_poll` 366 -> 367 | 685 | | Open |
| TGT-032 | dome_beaver | DOME_SHUTTER | on_change 780 | `dome_status_poll` 390 -> 396/401/406/410/414 | 783 | | Open |
| TGT-033 | dome_nexdome | DOME_SHUTTER | on_change 579 | `nexdome_update_shutter` 316-317 -> 337/341/346 | 582 | | Open |
| TGT-034 | dome_talon6ror | DOME_SHUTTER | on_change 476 | `dome_status_poll` 374 -> `talon6ror_mirror_state` 284/292 (and 300/308) | 480, 481 | `mirror_state` re-reads `state != OK` (282/290); a request copied after 374 is overwritten and set OK. | Open |
| TGT-035 | polaralign_mlastro | POLARALIGN_DIRECTION_AZ | on_change 533 | `mlastro_parse_telemetry` (on_timer 415) 257 -> 259 | 534 | Guard and write adjacent, tiny window. | Open |
| TGT-036 | polaralign_mlastro | POLARALIGN_DIRECTION_ALT | on_change 526 | same, 260 -> 262 | 527 | Same. | Open |
| TGT-037 | polaralign_mlastro | X_MLASTRO_BACKLASH_ENABLE | on_change 598 | same, 275 -> 277 | 599 | Same. | Open |
| TGT-038 | aux_upb | AUX_POWER_OUTLET | on_change 769 (.c:1232) | on_timer, `upb_adopt` guard 167 -> 169-186 | 770-773 | | Open |
| TGT-039 | aux_upb | AUX_USB_PORT (v2) | on_change 940 (.c:1241) | on_timer 199 -> 201-228 | 974 | | Open |
| TGT-040 | aux_upb | AUX_USB_PORT (v1 hub) | on_change 940 | on_timer libusb loop, guard + write 402-403, re-checked per port | 948-949 | The 3.0.0.29 fix. | Open |
| TGT-041 | aux_upb | AUX_DEW_CONTROL | on_change 921 (.c:1238) | on_timer 349 -> 352 | 922 | | Open |
| TGT-042 | aux_upb | X_AUX_HUB (v1) | on_change 1096 (.c:1244) | on_timer 190 -> 193 | 1097 | See TGT-D02 at 194. | Open |
| TGT-043 | aux_ppb | AUX_POWER_OUTLET | on_change 300 (.c:606) | on_timer 179/186 -> 182/189 | 301-303 | | Open |
| TGT-044 | aux_ppb | AUX_DSLR_POWER | on_change 322 (.c:609) | on_timer 222 -> 225-237 | 323-332 | | Open |
| TGT-045 | aux_ppb | AUX_DEW_CONTROL | on_change 400 (.c:615) | on_timer 207 -> 210 | 401 | | Open |
| TGT-046 | aux_svbpowerbox | AUX_POWER_OUTLET | on_change 427 (.c:731) | on_timer 288 -> 289-294 | 428-434 | | Open |
| TGT-047 | aux_svbpowerbox | AUX_USB_PORT | on_change 518 (.c:737) | on_timer 306 -> 307-308 | 519-520 | | Open |
| TGT-048 | aux_wbplusv3 | AUX_POWER_OUTLET | on_change 287 (.c:587) | on_timer 202 -> 203-204 | 288-289 | | Open |
| TGT-049 | aux_wbplusv3 | AUX_USB_PORT | on_change 326 (.c:593) | on_timer 198 -> 199 | 327 | | Open |
| TGT-050 | aux_wbprov3 | AUX_POWER_OUTLET | on_change 386 (.c:722) | on_timer 273 -> 274-276 | 387-389 | | Open |
| TGT-051 | aux_wbprov3 | AUX_USB_PORT | on_change 447 (.c:728) | on_timer 265 -> 266-270 | 448-452 | | Open |
| TGT-052 | aux_uch | AUX_USB_PORT | on_change 231 (.c:412) | on_timer 103 -> 105-131 | 233 | Same shape as upb v2. | Open |
| TGT-053 | aux_usbdp | AUX_DEW_CONTROL | on_change 426 (.c:699) | on_timer, `usbdp_adopt` 173 -> 175/177 | 427 | | Open |
| TGT-054 | aux_usbdp | AUX_LINK_CH_2AND3 | on_change 534 (.c:708) | on_timer 243 -> 245/247 | 535 | | Open |
| TGT-055 | aux_usbdp | AUX_HEATER_AGGRESSIVITY | on_change 552 (.c:711) | on_timer 225 -> 228-237 | 553-561 | The poll compares against the shadow `requested_aggressivity`. | Open |
| TGT-056 | aux_cloudwatcher | AUX_GPIO_OUTLETS | on_change 854 (.c:1450) | on_timer, guard + write 823-824 (guard deliberately after the read, comment 819) | 855 | | Open |
| TGT-057 | aux_mgbox | X_SEND_WEATHER_MOUNT | on_change 715 (.c:901) | `mgbox_process_line` CAL frame 216 -> 217 (from aux 490 and gps 776 timers) | 724 | The pending flag is set only inside the handler (723), so it does not cover copy -> handler. | Open |
| TGT-058 | aux_mgbox | X_SEND_GPS_MOUNT | on_change 815 (.c:1105) | same, 225 -> 226 | 824 | Same. | Open |

### A3. Another handler overwrites a queued request

Not a background path, but the same mechanism: a handler already on the device queue rewrites P after
P's request was copied, and P's handler then reads the rewritten value.

| ID | Driver | Property | Overwritten by | P's handler reads | Note | Status |
| --- | --- | --- | --- | --- | --- | --- |
| TGT-060 | ccd_atik | CCD_COOLER | CCD_TEMPERATURE handler 498 sets ON | 485 | A pending cooler OFF becomes ON. | Open |
| TGT-061 | ccd_mi | CCD_COOLER | CCD_TEMPERATURE handler 499 | 475 | Same. | Open |
| TGT-062 | ccd_qsi | CCD_COOLER | CCD_TEMPERATURE handler 622 | 613 | Same. | Open |
| TGT-063 | ccd_sx | CCD_COOLER | CCD_TEMPERATURE handler 1017 | poll 766 | Same; the poll applies the value. | Open |
| TGT-064 | ccd_atik2 | CCD_COOLER | CCD_TEMPERATURE handler 352 | 340 | Same; see also TGT-B06. | Open |
| TGT-065 | ccd_touptek | CCD_COOLER | CCD_TEMPERATURE handler .c:1400 (`_ANYTIME`, .c:1900) | .c:1380 | Same. Hand-written driver. | Open |
| TGT-066 | ccd_asi | X_PRESETS, CCD_MODE, X_PIXEL_FORMAT | `adjust_preset_switches` 127 from CCD_GAIN 1184 and CCD_OFFSET 1209; CCD_MODE / X_PIXEL_FORMAT written at 1244-1261, 1286-1288, 1341, 1372 | 1420-1428, 1278, 1364 | No mutual rejection between these properties. | Open |
| TGT-067 | ccd_playerone | X_PRESETS, CCD_MODE, X_PIXEL_FORMAT | 768/1192/1233 | 1272-1289, 1318-1320, 1383, 1418 | Same pattern as TGT-066. | Open |
| TGT-068 | ccd_svb | CCD_MODE, X_PIXEL_FORMAT | 1100-1117, 1142-1144 | 1197, 1231 | Same pattern. | Open |
| TGT-069 | ccd_qhy | X_PIXEL_FORMAT | 825 | 849 | | Open |
| TGT-070 | ccd_qhy2 | X_PIXEL_FORMAT | 917 | 941 | | Open |
| TGT-071 | ccd_simulator | FOCUSER_DIRECTION | `start_focuser_move` 1320 from the POSITION and STEPS handlers | 1377 | FOCUSER_DIRECTION is otherwise a synchronous selector; check whether the simulator's own handler needs it. | Open |
| TGT-072 | mount_synscan | MOUNT_USE_ENCODERS | autohome handler 722 sets false | 1883-1884 | Only when an encoders request is queued behind a pending autohome. | Fixed (3.0.0.12, 2026-09-26): the handler sends the targets and shows the encoder state the mount accepted (kept in private data, also cleared by auto home) with ALERT on failure; regression test `synscan_mount_encoders_request_survives_autohome`. |
| TGT-073 | dome_talon6ror | X_CLOSE_COND | `talon6ror_unpack_configuration` 245-247, run when an X_MOTOR_CONF / X_DELAY_CONF write fails (256) | 726-728 | A queued X_CLOSE_COND request is reverted. | Open |
| TGT-074 | aux_wbplusv3 | AUX_DEW_CONTROL | AUX_HEATER_OUTLET handler 343 sets MANUAL and publishes | handler .c:451 only publishes OK | A pending AUTOMATIC is silently lost and shown OK; the poll's auto-dew logic (215) reads the switch. | Open |
| TGT-075 | aux_wbprov3 | AUX_DEW_CONTROL | AUX_HEATER_OUTLET handler 490 | handler .c:562 | Same as TGT-074 (poll 292). | Open |

## B. Findings the switch target does not fix

| ID | Driver | Where | Finding | Needed fix | Status |
| --- | --- | --- | --- | --- | --- |
| TGT-B01 | ccd_pentax | `indigo_ccd_pentax.c` DSLR_ISO: copy 1036, `indigo_set_timer(iso_callback)` 1039, poll `pentax_get_full_state` 725 from `state_timer_callback` 888-893 (3 s), handler `iso_callback` 959 | No BUSY guard on accept; the handler and the poll run on independent timer threads; `PRIVATE_DATA->mutex` does not cover the copy. High. | BUSY guard and handler queue first, then the target. | Open |
| TGT-B02 | mount_rainbow | MOUNT_TRACKING (handler 462, reader write 222, state OK 223), MOUNT_TRACK_RATE (handler 469, reader write 228, state OK 229; also read by the EQ handler 364) | Written by a separate reader thread started at 281, truly concurrent with the bus thread and the handler; also forces the state OK. High. | Move the reader's property writes onto the device queue (or guard them), then the target. | Open |
| TGT-B03 | several | number `target` overwritten by a poll without a BUSY guard: focuser_qhy on_timer 540-544 (`FOCUSER_POSITION`, handler reads target 681); focuser_steeldrive2 on_timer 508-510 (handler 709); aux_asiair `asiair_update_pwm` 184-185 (on_timer 202) and aux_rpio `rpio_update_sensors` 190-191 (on_timer 225) for `AUX_GPIO_OUTLET_FREQUENCIES` / `AUX_GPIO_OUTLET_DUTY` (handlers asiair 378/403, rpio 473/498; `*_apply_pwm` reads target asiair 159-160, rpio 206-207); mount_rainbow reader writes MOUNT_GEOGRAPHIC_COORDINATES value and target 203/209 (handler reads value 339-346) and MOUNT_GUIDE_RATE value 234 (handler 479) | A pending number request is replaced by the device value. High. | Driver fix: the poll writes only `number.value`, or guards the write with BUSY. | Open; focuser_steeldrive2 Fixed (3.0.0.18, 2026-09-27): the focuser poll skips its status read and publication while a FOCUSER_POSITION or FOCUSER_STEPS request is pending (BUSY it did not publish itself for an external motion), `steeldrive2_read_aux()` leaves a BUSY AUX_HEATER_OUTLET alone, and the AUX_HEATER_OUTLET handler reads `number.target` and on failure shows the PWM the device last reported with ALERT; regression tests `position_request_survives_poll`, `aux_heater_request_survives_poll`. |
| TGT-B04 | 9 mounts | GUIDER_GUIDE_RA/DEC with `accept_while_busy`: the previous pulse's finalizer zeroes `number.value`, the new handler reads `number.value` and cancels the old finalizer only when it runs. ioptron 1958/1966, lx200 3288/3294, nexstar 1044-1045/1056-1057, nexstaraux 810/815, pmc8 940-941/951-952, simulator 480-481/490-491, starbook 396/402, synscan 2043-2044/2059-2060, temma 316/324 | A pulse arriving as the previous one ends is dropped (duration 0, reported OK). | Handlers read `number.target`, which the finalizer does not touch. | Open; simulator Fixed (3.0.0.22, 2026-09-26): the finalizers clear only `number.value` and the handlers restore the values from the targets before reading them; regression test `guider_pulse_survives_previous_finalizer`; temma Fixed (3.0.0.19, 2026-09-26): the finalizers already cleared only `number.value`, the handlers now restore the values from the targets before reading them; regression test `temma_guider_pulse_survives_previous_finalizer`; synscan Fixed (3.0.0.12, 2026-09-26): the finalizers already cleared only `number.value`, the handlers now restore the values from the targets before reading them; regression test `synscan_guider_pulse_survives_previous_finalizer`; nexstaraux Fixed (3.0.0.22, 2026-09-26): the finalizers already cleared only `number.value`, the handlers now restore the values from the targets before reading them; regression test `guider_pulse_survives_previous_finalizer`; nexstar Fixed (3.0.0.46, 2026-09-26): the finalizers already cleared only `number.value`, the handlers now restore the values from the targets before reading them; regression test `nexstar_guider_pulse_survives_previous_finalizer`; pmc8 Fixed (3.0.0.15, 2026-09-27): the finalizers already cleared only `number.value`, the handlers now restore the values from the targets before reading them; regression test `pmc8_guider_pulse_survives_previous_finalizer`. |
| TGT-B05 | several | Background writers forcing the property state over a pending BUSY: rainbow 198/223/229, synscan 787, pmc8 510/512, nexstaraux 533/538, nexstar 344 (MOUNT_UTC_TIME OK every poll), focuser_ioptron 130, focuser_steeldrive2 799, aux_wcv4ec timer | Reopens the `INDIGO_COPY_*_PROCESS_CHANGE` BUSY guard, so a second request is accepted while the first is still queued, and shows OK for a request not yet sent. | Fix together with the A1 row of the same driver: the writer leaves the state alone while BUSY. | Open; synscan Fixed (3.0.0.12, 2026-09-26): `synscan_clear_tracking_state` and the slew finalizer's tracking start (TGT-003) and the PPEC training timer (TGT-004) write only the value while the property is BUSY and leave state and publication to the pending handler; regression tests `synscan_mount_tracking_request_survives_home_finalizer`, `synscan_mount_pec_training_request_survives_training_end`; nexstaraux Fixed (3.0.0.22, 2026-09-26): the slew finalizer still starts tracking and writes the value at the end of a goto but leaves state and publication to a pending MOUNT_TRACKING request (TGT-006); regression test `tracking_request_survives_slew_end`; nexstar Fixed (3.0.0.46, 2026-09-26): the poll writes MOUNT_UTC_TIME items and state only while the property is not BUSY (TGT-C01); regression test `nexstar_utc_request_survives_poll`; pmc8 Fixed (3.0.0.15, 2026-09-27): `mount_goto_complete` (end of a GOTO or SYNC) still starts tracking and writes the value but leaves state and publication to a pending MOUNT_TRACKING request (TGT-005); regression test `pmc8_mount_tracking_request_survives_goto_end`; focuser_ioptron Fixed (3.0.0.10, 2026-09-27): `ioptron_publish()` (poll, motion finalizer, ABORT and X_FOCUSER_ZERO_SYNC handlers) and the poll's read error path `ioptron_read_error()` write FOCUSER_REVERSE_MOTION value and state only while it is not BUSY (TGT-010); regression tests `reverse_request_survives_poll`, `reverse_request_survives_poll_failure`; focuser_steeldrive2 Fixed (3.0.0.18, 2026-09-27): the aux poll sets and publishes the state of AUX_HEATER_OUTLET, X_USE_PID and X_USE_AUTO_DEW only while they are not BUSY (TGT-011, TGT-012), and the focuser poll no longer publishes FOCUSER_POSITION / FOCUSER_STEPS over a pending request (TGT-B03); regression tests `aux_pid_request_survives_poll`, `aux_auto_dew_request_survives_poll`, `aux_heater_request_survives_poll`, `position_request_survives_poll`; aux_wcv4ec Fixed (3.0.0.10, 2026-09-27): the status timer no longer sets AUX_COVER OK over a request that is BUSY before its handler started a move (TGT-015); regression test `cover_request_survives_detection_end`. |
| TGT-B06 | ccd_atik2 | on_timer 245/247 sets CCD_COOLER OK/ALERT from `status == CCD_COOLER_ON_ITEM->sw.value`; handler 340 | A queued cooler request can show ALERT and leave BUSY before it runs. | Fix with TGT-064. | Open |
| TGT-B07 | all guarded poll drivers | End-of-poll `INDIGO_UPDATE_PROPERTY_STATE(P, OK)` not re-checked (e.g. upb 418-452, ppb 241-255, uch 149-151, usbdp 274-294, cloudwatcher 825) | A request copied between the write and the publish is shown OK before its handler runs. Low. | Covered once the handler owns the final state; check per driver while fixing A2. | Open |
| TGT-B08 | dome_nexdome3 412, dome_nexdome 347, dome_beaver 411/415, dome_talon6ror 299/307 | Writers set DOME_SHUTTER BUSY on their own when motion is seen | The BUSY guard then silently drops client requests. | Decide per driver while fixing the A row. | Open |

## C. Text properties (MOUNT_UTC_TIME)

Text items get no target (design note section 8); the mount context target used by `mount_ioptron`
(`indigo_mount_set_utc_target()` / `indigo_mount_get_utc_target()`) is the fix.

| ID | Driver | Writer | Handler reads | Severity | Status |
| --- | --- | --- | --- | --- | --- |
| TGT-C01 | mount_nexstar | poll 397-398, no guard, state OK at 344 | 652/658 (via 815) | High | Fixed (3.0.0.46, 2026-09-26): the change branch records the request with `indigo_mount_set_utc_target()`, the handler sends `indigo_mount_get_utc_target()` and writes it into the items once the mount accepted it (ALERT otherwise), and the poll leaves a BUSY MOUNT_UTC_TIME alone; regression test `nexstar_utc_request_survives_poll`. |
| TGT-C02 | mount_rainbow | reader thread 187/197, no guard, state OK 198 | 452/456 | High (see also TGT-B02) | Open |
| TGT-C03 | mount_starbook | poll 426-428, no guard | 515/516 | High | Open |
| TGT-C04 | mount_lx200 | poll, guard 2539 -> 2540-2542 | 3231/3235 | Medium | Open |
| TGT-C05 | mount_nexstaraux | 468, no guard | synchronous handler | Low (flicker) | Won't fix: the driver never unhides MOUNT_UTC_TIME (hidden by `indigo_mount_attach()`), so the property is never defined, no client can send a request, and neither the driver nor the mount base has a change handler for it; the poll only refreshes an undefined property. |
| TGT-C06 | wheel_astroasis | X_CUSTOM_SUFFIX (handler 301, .c:489) overwritten by `astroasis_read_names` 87 from the X_FACTORY_RESET handler 378; X_BLUETOOTH_NAME 105 and the X_BLUETOOTH switch 95 (handler reads 325) on the same path | 301 | Low: needs a queued factory reset; the bluetooth properties are hidden and never defined. | Open |

## D. Other defects found during the audit

| ID | Driver | Where | Finding | Status |
| --- | --- | --- | --- | --- |
| TGT-D01 | aux_usbdp | 181, 214, 220 | Operator precedence: `usbdp_adopt(P) && a != x \|\| b != y \|\| c != z` guards only the first comparison, so AUX_HEATER_OUTLET, AUX_CALLIBRATION and AUX_DEW_THRESHOLD are overwritten without a guard whenever item 2 or 3 differs; their handlers read `number.value` (373-375, 478, 511). | Open |
| TGT-D02 | aux_upb | 194 | The X_AUX_HUB change sets `updateAutoHeater` instead of `updateHub`: the hub change is never published and AUX_DEW_CONTROL is published OK, possibly showing a pending dew request as done. | Open |
| TGT-D03 | aux_upb3 | on_connect 311/315 | The dew logic is inverted, so the device's dew state is never adopted. The poll never refreshes the outlet, USB or dew switches, so the display does not follow the device. | Open |
| TGT-D04 | aux_wcv4ec | 269, 282-284 | The DETECT handler publishes AUX_SET_OPEN_CLOSE instead of AUX_DETECT_OPEN_CLOSE; the read loop has no bound, so a lost reply blocks the device queue. | Fixed (3.0.0.10, 2026-09-27): the refusal publishes AUX_DETECT_OPEN_CLOSE with ALERT and the trigger released, and the reply is read with `indigo_uni_read_section2()` (5 s first byte, 1 s next bytes) for at most ten lines, ending with ALERT when no OpenSet / CloseSet came; regression tests `detection_refused_while_the_cover_moves`, `lost_detection_reply_does_not_block_the_queue` (simulator `--lose-detect-reply`). |
| TGT-D05 | wheel_qhy | on_connect 85 | The fallback sets X_MODEL to CFW1 and publishes without a guard; one-shot, the X_MODEL handler is trivial. Low. | Open |
| TGT-D06 | ccd_pentax | DSLR_PROGRAM, DSLR_APERTURE, DSLR_SHUTTER (poll 640, 678, 687/693/706) | RW properties without a change handler: client writes are ignored. | Open |
| TGT-D07 | framework (seen with mount_simulator) | `indigo_test` `test_mount_simulator_asan` on Linux | The sanitizer build reports a leak of about one light property per driver start-up and shutdown, allocated in `indigo_init_light_property` (libindigo). Reproduced with the unchanged 3.0.0.21 driver and test, so it predates this branch; not part of the recorded run. To be checked in `indigo_libs`, not in the drivers. | Open |
| TGT-D08 | mount_temma | `temma_update_position()` (poll, GOTO and park finalizers) | The side of pier read from the `E` reply is written into MOUNT_SIDE_OF_PIER but never published after connect, so clients keep seeing the side from connection time even when the mount changes side (for example after a GOTO across the meridian). Found while making the property read-only (3.0.0.21). | Fixed (3.0.0.22, 2026-09-27): `temma_update_position()` publishes MOUNT_SIDE_OF_PIER when the side read from the `E` reply differs from the last known side, only while connected and never for an unchanged side; regression test `temma_side_of_pier_change_is_published` (simulator control `side E` / `side W`). |
| TGT-D09 | focuser_prodigy | powerbox connect, `indigo_focuser_prodigy.c` ~681 vs ~686-687 (3.0.0.8) | AUX_POWER_OUTLET and AUX_USB_PORT are published before the generated connection handler defines them, so clients see an update of an undefined property on every powerbox connect. Found while fixing TGT-013/014; no test fails because of it. | Open |
| TGT-D10 | aux_wcv4ec | detect handler and `wcv4ec_read_status` (3.0.0.10) | The detect handler discards the serial input right after sending the detect command, so a very fast reply can be dropped (now a bounded ALERT instead of a hang); the status reader still reads lines without an inter-byte timeout. Found while fixing TGT-015/TGT-D04; the call order was kept because the rules require proof before reordering protocol calls. | Open |

## E. Already fixed another way (candidates for simplification with the target)

- `aux_upb`: `upb_adopt()` (68), a per-field BUSY check at parse time. The handlers reading
  `indigo_get_switch_target()` close the remaining window.
- `aux_usbdp`: `usbdp_adopt()` (80) and the `requested_aggressivity` shadow (225, 554-562).
- `aux_mgbox`: pending flags, sequence counters and finalizer confirmation (433-455).
- `aux_wcv4ec`: `operation_running` as a pending flag.
- `aux_arteskyflat`: the `light_on` shadow restores the switch on ALERT (80-82); no poll, but it is the
  "apply targets on success, device state on failure" pattern.
- `wheel_astroasis`: `config`, `custom_suffix` and `bluetooth_name` shadows restore values on failure
  (303, 327, 351).
- `ccd_ptp`: `handle_set_property` (`indigo_ccd_ptp.c:330`) re-copies the saved client request before
  `set_property`, so the device gets the request; `ptp_update_property` (`indigo_ptp.c:1523-1568`) only
  makes the displayed value flicker.
- CCD_COOLER in ccd_asi (570-575), ccd_playerone (462-467), ccd_svb (461-466), ccd_sx (774/776),
  ccd_qhy (475), ccd_qhy2 (547), ccd_fli (380): by design the handler only sets BUSY and the poll
  applies the value and sets OK.

## F. Checked without a candidate

- Mount, dome, rotator, GPS, polar align, AO: ao_sx, dome_simulator, dome_skyroof, gps_gpsd, gps_nmea,
  gps_simulator, mount_starbook (switches; its MOUNT_TRACKING is read-only), polaralign_simulator,
  rotator_asi, rotator_falcon, rotator_lunatico, rotator_optec, rotator_simulator, rotator_wa.
  system_alpaca has no source.
- CCD and guider: ccd_dsi, ccd_fli, ccd_iidc, ccd_ssag, ccd_uvc, the touptek wrappers (ccd_altair,
  ccd_baccam, ccd_bresser, ccd_mallin, ccd_meade, ccd_ogma, ccd_omegonpro, ccd_rising, ccd_ssg,
  ccd_svb2), guider_asi, guider_cgusbst4, guider_gpusb, focuser_mjkzz_bt and focuser_wemacro_bt (all
  handlers synchronous). The remaining CCD drivers only appear in A3 or E.
- Focuser: asi, askar, astroasis, astromechanics, dmfc, dsd, efa, fc3, fcusb, fli, focusdreampro,
  lacerta, lakeside, lunatico, mjkzz, moonlite, mypro2, nfocus, nstep, optec, optecfl, primaluce, qhy
  (switches; see TGT-B03), robofocus, usbv3, wemacro.
- AUX and wheel: aux_arteskyflat, aux_astromechanics, aux_dsusb, aux_fbc, aux_flatmaster, aux_flipflat,
  aux_geoptikflat, aux_joystick, aux_rts, aux_skyalert, aux_sqm, aux_upb3 (see TGT-D03), wheel_asi,
  wheel_astroasis (see TGT-C06), wheel_atik, wheel_fli, wheel_indigo, wheel_manual, wheel_mi,
  wheel_optec, wheel_playerone, wheel_qhy (see TGT-D05), wheel_quantum, wheel_sx, wheel_trutek,
  wheel_xagyl.
- No `_ANYTIME` / `accept_while_busy` switch property is also written in the background; the only
  overlaps are the GUIDER_GUIDE numbers in TGT-B04 and MOUNT_MOTION_RA/DEC (cleared only by ABORT, by
  design).

## Fix log

One commit per driver on `refactoring_targets`, each with the driver's version bump, a test that
reproduces the window where the suite can, the test record from `tools/run_driver_test.py`, and the
status of its rows above updated to `Fixed`.

### Merge of `refactoring` (2026-09-26, c6a7641)

`refactoring` regenerated 32 multi-device drivers after the generator change that makes the master
device its own `master_device` (8531b6c) and bumped them, reusing the version numbers this branch had
already given four drivers: mount_ioptron 59, mount_simulator 22, mount_synscan 12 and mount_temma 19.
The merged drivers carry both changes and are bumped once more to mount_ioptron 60, mount_simulator 23,
mount_synscan 13 and mount_temma 20, regenerated with the new generator and retested. The `Fixed
(3.0.0.N)` notes above name the version in which the fix was first made on this branch.
