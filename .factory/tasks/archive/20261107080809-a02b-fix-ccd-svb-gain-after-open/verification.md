# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: implementing agent (Claude Code, claude-fable-5-1); independent static review by a fresh subagent, see quality-review.md
Date: 2026-11-07
Revision and working-tree state: master 0d1e4672e plus the uncommitted task diff on task/20261107080809-a02b-fix-ccd-svb-gain-after-open
Review independence: the verifier also implemented the change; all test runs were made by the implementer

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `Gain before the first exposure` passes in the final recorded run: `CCD_GAIN` OK at 20, `config[SVB_GAIN]` = 20 after connect without any exposure (`starts` = 0); on the SV305PRO the hardware step `gain_before_first_exposure()` (a `CHECK` before the first exposure of the run) passed within the recorded 1/1 OK run |
| AC-002 | pass | same case: `exposure_written` = 1 after the connect |
| AC-003 | pass | the updated suite against the master driver (3.0.0.34): `Gain before the first exposure` FAILs at `wait_state(0, "CCD_GAIN", INDIGO_OK_STATE)`; `Matrix control_failures` (gain right after connect) and `Configuration roundtrip` (`CONFIG` LOAD restoring the saved gain) fail the same way, 46/49; on the SV305PRO with 3.0.0.34 the hardware step timed out ("Timeout: SVBONY SV305PRO CCD_GAIN expected state 1", `hardware gain before the first exposure: FAIL`) |
| AC-004 | pass | `svb_set_number()` logs `INDIGO_DRIVER_ERROR` with both SDK results and the value read back on failure and `INDIGO_DRIVER_DEBUG` on success (regenerated `.c`, inspected; not asserted by the suite) |
| AC-005 | pass | final recorded runs: fake SDK 49/49 OK, SVBONY SV305PRO 1/1 OK |
| AC-006 | pass | `version = 35;` in the `.driver`, `DRIVER_VERSION 0x03000023` in the regenerated `.c`, recorded as `3.0.0.35` |
| AC-007 | pass | `make -C indigo_test build/hardware/test_ccd_svb_hw` compiles the step (only the pre-existing SDK dylib version linker warnings); the step ran on the SV305PRO in both hardware runs |

## Executed checks
All commands ran on macOS arm64 (Darwin 27.0.0, Apple clang 21) in /Users/polakovic/Development/indigo unless noted; the attached camera is an SVBONY SV305PRO (USB, no guider port cable).

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make -C indigo_test build/integration/test_ccd_svb_sdk && ./build/integration/test_ccd_svb_sdk` (baseline, master sources) | repository root / `indigo_test` | 0 | 48 cases, "all tests passed" |
| `make -C indigo_test build/integration/test_ccd_svb_sdk` (fake SDK refusal and the new case, master driver `.c`) and the binary | `indigo_test` | 1 | reproduction: `Gain before the first exposure`, `Matrix control_failures` and `Configuration roundtrip` FAIL, 46/49 PASS (details under AC-003) |
| `make -C indigo_test build/hardware/test_ccd_svb_hw && make -C indigo_test test-ccd-svb-hw` (updated hardware step, master driver archive 3.0.0.34) | repository root | 2 | hardware reproduction on the SV305PRO: the gain write before the first exposure times out in ALERT, `FAIL Physical camera exposure, streaming, guider and reconnect` |
| `make -C indigo_drivers/ccd_svb -f ../../Makefile.drv all` (edited `.driver`, twice) | repository root | 0 | `indigo_ccd_svb.c` regenerated and built; `git diff` of the `.c` shows only the version define, `svb_set_number()` and the `on_connect` block |
| `python3 tools/run_driver_test.py ccd_svb` (final) | repository root | 0 | 49/49 OK; README.md line `2026-11-07 09:19 3.0.0.35 mac arm64 fake SDK 49/49 OK`; TEST_SUMMARY.md regenerated |
| `python3 tools/run_driver_test.py ccd_svb --hw` (final, non-interactive, no hot-plug case) | repository root | 0 | 1/1 OK on the SV305PRO (exposure, formats, ROI, bins, gain/offset/config roundtrip, reject guards, 85-frame stream, reconnect, driver reinitialization); README.md line `2026-11-07 09:19 3.0.0.35 mac arm64 SVBONY SV305PRO 1/1 OK` |
| `make -C indigo_test test-clean` | repository root | 0 | test binaries removed |
| `./.factory/scripts/findings check` | repository root | 0 | see the register reconciliation |

## Baseline comparison and regressions
No regression: the 48 cases of master pass before and after the change; the new case is the 49th. Two existing
cases (`Matrix control_failures`, `Configuration roundtrip`) now exercise the SDK refusal through the fake SDK
and pass only with the exposure write at connect.

## Found defects
| Defect | Found by | Impact | Fix | Regression test |
| --- | --- | --- | --- | --- |
| the generator migration (3.0-8) dropped the 3.0-7 exposure write at connect, so the SDK refuses every gain request before the first exposure (SV705C, SV905C2 of the user; SV305PRO here) and a saved gain turns the configuration restore into ALERT, which makes the Imager Agent refuse the camera | user report; reproduced on the fake SDK and on the SV305PRO | gain unusable before the first frame; camera refused by agents with a saved configuration | `SVBSetControlValue(id, SVB_EXPOSURE, 1 s)` after a successful `initialize_camera()` in `on_connect` | `gain_is_accepted_before_the_first_exposure`, `configuration_roundtrip`, `control_failures`, hardware step `gain_before_first_exposure()` |
| `svb_set_number()` published ALERT without any log line, so `-vv` showed nothing for the refused request | user report | undiagnosable in the field | error/debug log lines with the SDK result codes | inspection |

## Manual checks and limitations
- The SV705C and SV905C2 of the report are not available; the hardware run covers the SV305PRO, which showed the
  same refusal with 3.0.0.34 and accepts the gain with 3.0.0.35.
- The hot-plug case of the hardware suite was not run (non-interactive run without `--hot-plug`).
- Linux and Windows were not run; the change uses only the SDK call the driver already makes elsewhere.
- Fake SDK addition from hardware observation (indigo_test/AGENTS.md): the gain refusal between open and the first
  exposure write comes from the user's SV705C/SV905C2 report and the SV305PRO reproduction above.

## Verification verdict rationale
The defect is reproduced hardware-free and on hardware against the pre-fix driver, every acceptance criterion
passes in the final recorded runs, the pre-existing cases pass, and the independent review found no blocking issue
(quality-review.md).

## Test summary
- Simulated tests: 49 run, 49 passed (final recorded run).
- Hardware tests: 1 run, 1 passed (SVBONY SV305PRO, final recorded run).

## Register reconciliation
No canonical FND records were created or changed.
