# Verification report

Verdict: pass
Reviewer: coordinator (implementer); independent code review by a fresh read-only subagent, see quality-review.md
Date: 2026-10-25
Revision and working-tree state: task branch from master 4387df2c0 with the uncommitted final candidate; identities in process.json `reviewed_content`
Review independence: acceptance checks run by the implementer; not independent

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `sony_ignored_press` and `sony_long_exposure_noise_reduction` pass, each failed in its negative control; on hardware the retry delivered the image (4 times in a 30-iteration loop, 2 times in the 20-iteration loop on the final candidate) |
| AC-002 | pass | `sony_fast_reopen` passes, failed with `PTP_SONY_REOPEN_DELAY` 0; hardware: 1 s gap 30/30 sessions without DeviceBusy (about one in four before), final loops 30/30 and 20/20 |
| AC-003 | pass | recorded hardware run 15/15 OK, 2026-10-25 14:38 UTC (Pi clock), 3.0.0.59 |
| AC-004 | pass | recorded fake run 168/168 OK (161 fake USB + 7 ICA), 15:39 local; ASan/UBSan fake USB and ICA suites pass |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `python3 tools/run_driver_test.py ccd_ptp --hw --device "Sony Alpha A7RIV"` | Pi 5 `~/indigo`, Debian 12 aarch64, sources hash-identical to the candidate | 0 | 15/15 OK, recorded in README |
| `python3 tools/run_driver_test.py ccd_ptp` | repo root, macOS arm64 | 0 | 168/168 OK, recorded in README |
| `make -C indigo_test test-ccd-ptp-sdk-sanitize` | repo root, macOS arm64 | 0 | all fake USB cases pass (an earlier run found the fake's stale press time, fixed) |
| `make -C indigo_test test-ccd-ptp-ica-sanitize` | repo root, macOS arm64 | 0 | 7/7 |
| `INDIGO_TEST_CASE_FILTER=<case> make -C indigo_test test-ccd-ptp-sdk` with the fix removed | repo root | 2 | negative controls: `sony_ignored_press` (driver files at HEAD), `sony_fast_reopen` (delay 0), `sony_long_exposure_noise_reduction` (undoubled window) fail |
| `./.factory/scripts/security` | repo root | 0 | Factory boundaries valid |
| `/tmp/diag_ptp --run` (Pi-only loop, discovery + N reconnect/0.01 s exposure, built from the hardware suite) | Pi | various | 3.0.0.58: 6/6 runs failed; experiments: hold 100 ms failed, settle 2 s before disconnect failed, 1 s gap 30/30; candidate 30/30, final candidate 20/20 |

## Baseline comparison and regressions
Previous recorded runs: hardware ILCE-7RM4 15/13 Failed (3.0.0.58), fake 165/165. Now 15/15 and 168/168 (three new cases). Pre-existing `-Wstringop-overflow` warning at `indigo_ptp.c:1897` unchanged.

## Findings
FND-20261025113906-c862-ptp-fuji-hardware-test-001 fixed: two causes (ignored press, busy session after a fast reopen), see the driver REFACTOR.md section "Sony ILCE-7RM4 reconnect capture fix".

## Manual checks and limitations
Physical hot-plug not run; LENR behaviour modelled from the camera's documented default, not measured; the cause of the ignored press is unknown (detected and retried); the reopen delay is measured on the ILCE-7RM4 only.

## Verification verdict rationale
All acceptance criteria verified on the final candidate in hardware and hardware-free runs.

## Register reconciliation
FND-20261025113906-c862-ptp-fuji-hardware-test-001 archived as fixed with this report as repair evidence.
