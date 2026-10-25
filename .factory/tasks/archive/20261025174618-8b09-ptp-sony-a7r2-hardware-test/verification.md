# Verification report

Verdict: pass
Reviewer: coordinator (implementer); independent code reviews by fresh read-only subagents, see quality-review.md
Date: 2026-10-25
Revision and working-tree state: task branch from master 70d34909b, checkpoint 5e9a17c2b plus the uncommitted final candidate; identities in process.json `reviewed_content`
Review independence: acceptance checks run by the implementer; not independent

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `sony_liveview_after_capture` (failed on the old driver); hardware live view cases pass |
| AC-002 | pass | `sony_old_api_aborted_shot`, `_idle`, `sony_old_api_lost_shot_expires` (negative controls failed); hardware `Abort and reacquire` discarded the aborted shot |
| AC-003 | pass | hardware `Control readback`, image cases and restore at exit |
| AC-004 | pass | recorded hardware run 15/15 OK, 2026-10-25 19:07 UTC (Pi clock), 3.0.0.60, no tracing |
| AC-005 | pass | recorded fake run 177/177 OK (170 fake USB + 7 ICA), 20:12 local; ASan/UBSan fake USB and ICA pass |
| AC-006 | pass | `sony_old_api_ignored_press` (failed with the retry limited to NEW_API); traced hardware loop: two ignored presses retried |
| AC-007 | pass | `sony_old_api_late_setting_write` (failed with 1 s rewrites), `sony_old_api_dropped_setting_write` (failed without old-API confirmation), `sony_old_api_step_not_rewritten` (failed without `write_unconfirmed`) |
| AC-008 | pass | `sony_old_api_leftover_drained` (failed without the drain) |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `python3 tools/run_driver_test.py ccd_ptp --hw --device "Sony Alpha A7RII"` | Pi 5 `~/indigo`, Debian 12 aarch64, sources hash-identical to the candidate | 0 | 15/15 OK, recorded |
| `python3 tools/run_driver_test.py ccd_ptp` | repo root, macOS arm64 | 0 | 177/177 OK, recorded |
| `make -C indigo_test test-ccd-ptp-sdk-sanitize test-ccd-ptp-ica-sanitize` | repo root, macOS arm64 | 0 | all pass |
| `INDIGO_TEST_CASE_FILTER=<case> make -C indigo_test test-ccd-ptp-sdk` with one fix disabled | repo root | 2 | negative controls listed above fail |
| `./.factory/scripts/security`, `./.factory/scripts/findings check` | repo root | 0 | pass |
| Pi-only traced loops (`/tmp/diag_ptp`, discovery + selected cases) | Pi | various | diagnosis; final traced image loop 13/13; not counted |

## Baseline comparison and regressions
Baseline on 3.0.0.59: 2/15 (stepping controls). Checkpoint 5e9a17c2b: recorded 15/11 Failed, fake 172/172. Final: 15/15, 177/177. Pre-existing `-Wstringop-overflow` warning at `indigo_ptp.c:1897` unchanged.

## Findings
All defects found in this run fixed (D-S2-1 to D-S2-5 in the driver REFACTOR.md); no finding records created.

## Manual checks and limitations
Physical hot-plug not run; Bulb not offered in the test setup; ILCE-7RM4 and X-T1 not rerun (NEW_API: only the live view window changed). Owner asked to limit camera runs (battery); 11 diagnostic or pre-verification runs were made before that request.

## Verification verdict rationale
All acceptance criteria verified on the final candidate.

## Register reconciliation
No finding records.
