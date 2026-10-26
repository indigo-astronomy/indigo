# Verification report

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5)
Date: 2026-10-26
Revision and working-tree state: merged candidate 9cab0070f (all six groups) for the full run; follow-up fix 1619d4243 retested as listed; MIGRATION_STATUS.md count update c13c69594 (documentation only). master e7cbb62a6 is an ancestor.
Review independence: the coordinator did not write the group fixes (six implementer subagents did, see [group-reports.md](group-reports.md)) but wrote the aux_upb follow-up 1619d4243. Every implementer captured its own pre-fix failures; the coordinator reran all checks on the merged candidate.

## Acceptance evidence
Per-finding fix, commit, regression and pre-fix evidence are in [group-reports.md](group-reports.md); this table records the merged-candidate result.

| Finding | Result | Evidence on the merged candidate |
| --- | --- | --- |
| bc8d-003, bc8d-016, bc8d-035, bc8d-036, bc8d-029, bc8d-011, bc8d-013 | pass | `test-unit` (test_uni_io, test_driver_loader, test_bus_helpers, test_test_runner, test_align_math) |
| 3bd2-015 | pass | `test-unit` (test_mount_alignment_load); mount_simulator case `mount_multi_point_alignment_passes_coordinates_through` PASS |
| bc8d-004 | pass | `test-unit` (test_recording_io); test_ccd_file_names covered by the implementer run (2/2), library change re-exercised by ccd_simulator 26/26 |
| bc8d-007, bc8d-008, bc8d-009 | pass | `test-unit` (test_raw_image); `test-raw-image-sanitize` exit 0 |
| bc8d-010 | pass | `test-unit` (test_image_math) |
| bc8d-037, bc8d-038 | pass | `test-indigo-js-formatting` exit 0 |
| bc8d-040 | pass | `test-server-static-driver` exit 0 (also after 1619d4243) |
| 60d4-009 | pass | `test-unit` (test_uni_io compress cases); `test-server-security-sanitize` exit 0 |
| 3bd2-032 | pass | mount_ioptron 137/137 (second full run; see limitations) |
| 3bd2-018 | pass | aux_svbpowerbox 16/16 |
| 3bd2-016 | pass | aux_usbdp 21/21 |
| 3bd2-020 | pass | ccd_simulator 26/26 |
| bc8d-028 | pass | rotator_falcon 14/14 |
| bc8d-025 | pass | focuser_lakeside 43/43 |
| FND-000048 | pass | aux_upb 52/52 before and after 1619d4243 |
| bc8d-044 | pass by inspection and compile | ccd_dsi has no fake/simulator; libdsi.c compiles in `make all` |
| bc8d-024 | pass | wheel_quantum 10/10, wheel_trutek 12/12, wheel_xagyl 12/12, wheel_asi 6/6, wheel_playerone 13/13, mount_pmc8 37/37, mount_temma 28/28, focuser_nfocus 17/17, mount_nexstar 38/38; focuser_optec new case `blocking_move_endless_reports` PASS, suite has pre-existing failure |
| bc8d-023 | pass | wheel_trutek 12/12, wheel_xagyl 12/12, wheel_qhy 8/8 |
| bc8d-017 | pass | aux_fbc 13/13 |
| bc8d-018 | pass | aux_flipflat 16/16 |
| 3bd2-023, bc8d-045 | pass by inspection | Arduino firmware simulators; arduino-cli not installed, not compiled or flashed |

## Executed checks
All in /Users/polakovic/Development/indigo, macOS 27 arm64 (universal x86_64/arm64 build), after `make -k -j8 all` (exit 0, no error lines; regeneration left no diff in generated driver files).

| Command | Exit code | Result |
| --- | --- | --- |
| `make -C indigo_test test-unit` | 0 | 385 PASS lines, 0 FAIL |
| `make -C indigo_test test-indigo-js-formatting` | 0 | pass |
| `make -C indigo_test test-server-static-driver` | 0 | 2/2, rerun after 1619d4243 also pass |
| `make -C indigo_test test-server-security-sanitize` | 0 | pass |
| `make -C indigo_test test-imager-download` | 0 | pass |
| `make -C indigo_test test-raw-image-sanitize` | 0 | pass |
| `python3 tools/run_driver_test.py <driver> --no-record --no-build` for the 21 drivers in the table | 0 except below | counts in the table |
| same, mount_ioptron | 1, then 0 | first run 132/137 (5 `ioptron_manual_motion_*` cases at test line 1469); filtered rerun 6/6; second full run 137/137 |
| same, focuser_optec | 1 | 42/43, `descriptor_cleanup` fails |
| same, mount_simulator | 1 | 27/28, `mount_abort_allows_fresh_goto` fails |

## Baseline comparison and regressions
Baseline built from master e7cbb62a6 in a temporary detached worktree:
- focuser_optec `descriptor_cleanup`: 3 of 3 filtered runs fail on master — pre-existing, not introduced.
- mount_simulator `mount_abort_allows_fresh_goto`: 1 of 4 filtered runs fail on master — pre-existing intermittent failure; the G1 implementer saw the same rate before its change.
- mount_ioptron `ioptron_manual_motion_*`: failed once in the full candidate run and passed in a filtered run and a second full run; not reproduced, intermittent.
Each is registered as a new finding (see Findings). Every new regression was observed failing before its fix by the implementers ([group-reports.md](group-reports.md)), except temma/nexstar deadlines, the DSI change and the Arduino files.

## Findings
New findings from this task are recorded in the registry: FND-20261025211159-e308-fix-audit-findings-batch-001 (focuser_optec descriptor_cleanup), -002 (mount_simulator abort intermittent), -003 (mount_ioptron manual motion intermittent), -004 (Temma/NexStar deadline branches untested), -005 (Lakeside stall bound depends on report cadence), -006 (NexStar GOTO completion has no overall deadline). Quality review follow-ups are in [quality-review.md](quality-review.md).

## Manual checks and limitations
- No hardware run. ccd_dsi, mount_temma/mount_nexstar timeouts and the Arduino simulators are verified by inspection/compile only.
- Windows and Linux builds not run. Linux-only aux_asiair/aux_rpio ASSERT_NEAR users were not run.
- The aux_upb follow-up 1619d4243 has no dedicated regression (the simulator cannot fail SH); it is checked by inspection and the 52/52 suite.
- README test records were deliberately not written (`--no-record`); this was not a requested test run.

## Verification verdict rationale
All 31 findings have source fixes. 28 are covered by regressions that pass on the merged candidate and failed before the fix (bc8d-024 lacks a timeout regression for two of its ten drivers). bc8d-044, 3bd2-023 and bc8d-045 are verified by inspection/compile only. The remaining suite failures reproduce on master or are intermittent, and are tracked as new findings.

## Register reconciliation
The 31 findings in brief.md are closed as `fixed` with this report and [group-reports.md](group-reports.md) as evidence. New findings -001 to -006 originate in this task.
