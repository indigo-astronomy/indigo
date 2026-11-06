# Verification report

Verdict: pass
Reviewer: implementing agent (Claude Code, claude-opus-5-5); independent static diff review by a fresh subagent, see quality-review.md
Date: 2026-11-06
Revision and working-tree state: base c180d7b32089cfda28df1417b96567546b276f58 plus the uncommitted task diff in /home/rumen/work/indigo-task-mount-touptek
Review independence: the verifier also implemented the change. The independent review was static; all test runs were done by the implementer.

Test mode: non-interactive simulator run (owner decision D7). No hardware (D6).

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `lx200_touptek_profile` (autodetected from `:GVP#`), `lx200_touptek_connect_publishes_the_device_state` (non-default parked state published, no foreign command), `lx200_touptek_bootloader_is_refused` |
| AC-002 | pass | `lx200_touptek_goto_sync_and_refusals`: GOTO BUSY→OK at the target; `e5#` on `:MS#` ALERT with the code; `e2#` on `:Sd` stops before `:MS#` |
| AC-003 | pass | same case: sync `N/A#` OK, `e9#` ALERT, no slew |
| AC-004 | pass | `lx200_touptek_profile`: R1/R4/R7/R9, `:Mn#`/`:Qn#`; abort cases in the park and home tests |
| AC-005 | pass | `lx200_touptek_profile`, `lx200_touptek_rates_buzzer_and_meridian` (refused `:Td#` ALERT showing ON, solar rate read back after reconnect) |
| AC-006 | pass | `lx200_touptek_rates_buzzer_and_meridian` (`:Sgr0.70#`, both axes 70, read back after reconnect), `lx200_touptek_guider_pulses_and_timing` |
| AC-007 | pass | `lx200_park_touptek`, `lx200_touptek_park_unpark_and_park_position`, `lx200_touptek_park_that_never_starts_ends_alert` |
| AC-008 | pass | `lx200_home_touptek`, `lx200_touptek_home_and_home_search` (search at home lasts until the mount moved, abort ends AWAY, a dropped `:MH#` ends ALERT) |
| AC-009 | pass | `lx200_touptek_site_and_time_signs` (exact `:St`/`:Sg`/`:SG`/`:SC`/`:SL` strings and order, refusal, readback after reconnect) |
| AC-010 | pass | `lx200_touptek_rates_buzzer_and_meridian`, `lx200_touptek_side_of_pier_and_altaz_mode` |
| AC-011 | pass | `lx200_touptek_side_of_pier_and_altaz_mode` |
| AC-012 | pass | Fault-injection cases above. The `e<n>#` decoding is exercised on `:MS#`, `:Sd`, `:CM#`, `:SBz`, `:SPc#` |
| AC-013 | pass | Full suite 163/163, including all 150 pre-existing cases (baseline 150/150) |
| AC-014 | pass | Version 83; `indigo_generator` output regenerated from the `.driver`; PROTOCOL.md, PROPERTIES.md and the MIGRATION_STATUS.md count (153 → 166) updated |

## Executed checks
All commands ran in the task worktree on Linux x86_64 (Ubuntu 24.04, gcc 13, git 2.43).

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make init && make -C indigo_libs all && make -C indigo_tools all` | worktree | 0 | library and generator built |
| `python3 tools/run_driver_test.py mount_lx200 --no-record` (original driver, baseline) | worktree, `.driver` at c180d7b32 | 0 | `mount_lx200: 150/150 OK` |
| `build/bin/indigo_generator indigo_mount_lx200.driver` | `indigo_drivers/mount_lx200` | 0 | `.c` regenerated; `.h` and `_main.c` unchanged; the `GUIDER_RATE` warning predates this change |
| `python3 tools/run_driver_test.py mount_lx200 --no-record` (first run with dialect) | worktree | 1 | 161/163; two test expectations corrected (see below) |
| `./build/integration/test_mount_lx200_simulator <case>` (review fixes, affected cases) | `indigo_test` | 0 | 5 ToupTek cases PASS |
| `python3 tools/run_driver_test.py mount_lx200` (final, recorded) | worktree | 0 | `mount_lx200: 163/163 OK`, recorded `2026-11-06 08:43 3.0.0.83 linux x64 simulator 163/163 OK` |
| `make build/integration/test_mount_lx200_simulator_sanitize && ./build/integration/test_mount_lx200_simulator_sanitize` | `indigo_test`, ASan + UBSan | 0 | 163/163 PASS, no sanitizer report |

Build warnings: the driver compiles without new warnings. Two `-Wformat-truncation` warnings in `meade_read_meade_firmware` (Meade
code) also appear when the original `.c` is compiled the same way.

## Baseline comparison and regressions
150/150 before and 150/150 of the pre-existing cases after, so no regression. Two expectations were corrected, each with a documented reason:
- `lx200_driver_metadata_and_base_properties` asserted 16 `X_MOUNT_TYPE` items. The new `TOUPTEK` item makes 17. The comment in the test states this.
- `lx200_touptek_site_and_time_signs` (new) counted `:SG+02:00#` absolutely. The connect also writes the host clock, with the same offset on this host, so the case now counts the increase.

## Guider timing
`lx200_touptek_guider_pulses_and_timing`, simulator software timing from the simulator receipt to the public OK callback, idle, 20/100/500 ms,
4 directions, n = 24: min 50.050, mean 50.122, median 50.113, max 50.225, stddev 0.038 ms. Signed mean error 103.6 %, maximum absolute
error 250.9 % (driven by the 20 ms pulses). The constant ~50 ms comes from the shared 50 ms pause after every command
(`meade_no_reply_command`), which happens before the finalizer is scheduled. It predates this change: the Meade case in the baseline shows a
median of 50.114 ms. No physical relay timing was measured.

## Found defects
| Defect | Found by | Impact | Fix | Regression test |
| --- | --- | --- | --- | --- |
| A home search started at home could end on the first poll before moving | independent review (source audit; the simulator first hid it) | `MOUNT_HOME` OK while the search had not run | pending/moved latch | `lx200_touptek_home_and_home_search` (duration > 1.5 s) |
| A home that never starts stays BUSY | review | client waits forever | start timeout → ALERT | same case, dropped `:MH#` |
| Guide rate set to 0.01 precision, read back to 0.1 | review | the rate changes after a reconnect | step 10 | `lx200_touptek_rates_buzzer_and_meridian` |
| Failed `:MP1#` write leaves a pending park | review | duplicate alert | clear pending | source audit only (a transport failure during the write is not injectable) |

INDI reference defects N1–N9 (protocol.md) are not reproduced: the dialect follows TMount.

## Sanitizer
The ASan/UBSan build of the full suite passed 163/163 with no AddressSanitizer, LeakSanitizer or runtime-error output.

## Manual checks and limitations
- No hardware (D6). The literal `:GVP#` (`SH20`) and `:GG#` replies, the reply form of the one-character set commands, the `:GU#` flag
  timing and the completion of `:MFH#` are modelled from protocol.md and are not hardware-verified.
- TCP default port 8080 is not exercised with the touptek model. The `lx200://` transport itself is covered by the existing opt-in TCP cases (not run).
- macOS and Windows were not run. The change has no platform-specific code.
- After the recorded run, the owner approved README.md text additions (the SH20 dialect, port 8080, MLAstro SAL33 (OnStep) as a tested mount). They change no code, so the run was not repeated.

## Verification verdict rationale
All acceptance criteria pass in the simulator, and there are no regressions in the 15 existing dialects. The remaining limitations come from having no hardware.

## Test summary
- Simulated tests: 163 run, 163 passed (final recorded run); sanitizer build 163 run, 163 passed.
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
No canonical FND records were created or changed. The review findings were fixed in-task (quality-review.md).
