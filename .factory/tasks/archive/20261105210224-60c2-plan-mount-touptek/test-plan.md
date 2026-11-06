# Test and regression plan

Status: complete for the simulator-validated delivery (D6: no hardware)

All cases are in `indigo_test/integration/test_mount_lx200_simulator.c` and run against `mount_lx200_simulator --model touptek`
(the simulator options are listed per case). The expected results come from protocol.md and the specification, not
from the implementation.

## Coverage matrix
| Scenario | Acceptance / preserved contract | Test | Setup | Expected result |
| --- | --- | --- | --- | --- |
| T-001 | AC-001, AC-004, AC-005, AC-003 | `lx200_touptek_profile` (`check_profile(14)`) | autodetect | Detected as TOUPTEK from `:GVP#`. Vendor `ToupTek`, 3 info items, standard properties defined. Sync `:Sr`/`:Sd`/`:CM#`; tracking `:Te#`/`:Td#`; rates `:TS#`/`:TL#`/`:TQ#`; slew rates R1/R4/R7/R9; motion `:Mn#`/`:Qn#`; reconnect; no update of an undefined property |
| T-002 | AC-001 | `lx200_touptek_connect_publishes_the_device_state` | `--touptek-parked` (non-default state) | Model `SH20`, firmware `1.0.3`, parked, lunar rate, buzzer LOW, guide rate 30, meridian 1/1/+5, home 3 items, site east positive. `:GEM#` sent once. No command of another dialect sent |
| T-003 | AC-002, AC-003, AC-012 | `lx200_touptek_goto_sync_and_refusals` | default; injected `e5#` on `:MS#`, `e2#` on `:Sd`, `e9#` on `:CM#` | GOTO BUSY→OK at the target; each refusal ALERT with the code in the message; a refused target sends no `:MS#`; the next request is taken |
| T-004 | AC-005, AC-006, AC-010 | `lx200_touptek_rates_buzzer_and_meridian` | default; injected `e1#` on `:SBz0`, `0` on `:Td#` | `:Sgr0.70#`, both axes 70; `:SBz0/2/1#`; refused volume ALERT showing the kept one; `:STa01+00#`, `:STa01-07#`, `:STa11-07#`; refused tracking ALERT showing ON; solar rate, buzzer, guide rate and meridian read back after reconnect |
| T-005 | AC-007, AC-001 | `lx200_touptek_park_unpark_and_park_position`, `lx200_park_touptek` | default; injected `e4#` on `:SPc#` | `:MP1#` BUSY → OK parked from `:GPS#` 2; motion and GOTO refused without a command; parked after reconnect; `:MP0#` unparks without motion; `:SPc#` OK, refused ALERT |
| T-006 | AC-007 | `lx200_touptek_park_that_never_starts_ends_alert` | `--touptek-park-stalls` | ALERT "did not start" within the start timeout, shown unparked; an abort of a pending park settles OK and nothing latches later |
| T-007 | AC-008 | `lx200_touptek_home_and_home_search`, `lx200_home_touptek` | default | `:MH#` and `:MFH#` BUSY → OK at `H`; search allowed at home; abort ends AWAY and stays AWAY |
| T-008 | AC-009 | `lx200_touptek_site_and_time_signs` | default; injected `0` on `:SG-05:00#` | `:St+48*30:00.000#`, `:Sg+017*15:00.000#`, `:St-33*45:00.000#`, `:Sg-020*00:00.000#`; `:SG+02:00#` before `:SC09/16/26#` before `:SL01:30:45#`; refused offset ALERT and no clock written; site and offset 2 read back after reconnect |
| T-009 | AC-011, AC-010 | `lx200_touptek_side_of_pier_and_altaz_mode` | default; injected `:Gm#` reply; `--touptek-altaz` | `0`/`1` mapped to EAST/WEST and back on the next poll; alt-az: mode ALTAZ, no side of pier or meridian properties, `:Gm#`/`:GTa#` never sent |
| T-010 | AC-001 | `lx200_touptek_bootloader_is_refused` | `--touptek-bootloader` | CONNECTION ALERT, disconnected, "firmware update mode" message, no mount property defined, no poll |
| T-011 | AC-006 | `lx200_touptek_guider_pulses_and_timing` | guider device only | `:Mg[nsew]DDDD#` for 20/100/500 ms; timing statistics printed (simulator software timing only) |
| T-012 | AC-013 | whole `test_mount_lx200_simulator` suite and its sanitizer build | all 15 existing profiles | Unchanged pass |

## Baseline evidence
Before the change: `python3 tools/run_driver_test.py mount_lx200 --no-record` in the task worktree. The result is recorded in verification.md.

## Bug reproduction
Not applicable for the dialect itself (new feature). Defects of the INDI reference (N1–N9) are not reproduced; the dialect follows
TMount, and the tests assert the TMount behavior (for example `:MS#` → `0#` success, ASCII `:GT#`/`:GBz#`).

## Negative cases and permission boundaries
Refusals (`e<n>#`, `0`) on `:MS#`, `:Sd`, `:CM#`, `:SBz`, `:Td#`, `:SPc#` and `:SG`; a park that never starts; a bootloader;
requests made while parked; an abort during a park or a home search.

## Isolation and cleanup
Every case starts its own simulator and driver instance and stops both in `cleanup`. Fault injection uses the existing
atomic control-file rename.

## Deferred tests and coverage limitations
- Hardware: none (D6). The literal `:GVP#`/`:GG#` replies, the reply form of the one-character set commands, the `N` and `H`
  timing of `:GU#` and the completion of `:MFH#` are modelled from protocol.md and are not hardware-verified.
- TCP transport: the dialect's default port 8080 is exercised only through the existing generic TCP cases, not with the
  touptek model (the shared `--tcp` path is dialect independent).
- Bluetooth LE is not supported by the driver.
