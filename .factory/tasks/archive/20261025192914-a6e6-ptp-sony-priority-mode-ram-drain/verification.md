# Verification report

Verdict: pass
Reviewer: coordinator (implementer); independent code reviews by fresh read-only subagents, see quality-review.md
Date: 2026-10-25
Revision and working-tree state: task branch from master 24319f3e6, uncommitted final candidate; identities in process.json `reviewed_content`
Review independence: acceptance checks run by the implementer; not independent

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | hardware "Reconnect during exposure": image after the reconnect EXIF 0.01 s (recorded run 20:55 UTC); fake `sony_shot_across_reconnect`, `_7rm4` (failed with the record reset on reconnect, `_7rm4` also before the access rule) |
| AC-002 | pass | generic drain path exercised by `sony_old_api_leftover_drained` and the traced hardware run (stale shot drained before the press) |
| AC-003 | pass | fake `sony_priority_mode_written` (failed without the write); hardware trace: write OK, read back 1 |
| AC-004 | pass | fake `sony_old_api_lost_shot_expires` (failed without expiry) |
| AC-005 | pass | recorded hardware 16/16 OK; recorded fake 180/180 OK; ASan/UBSan fake USB and ICA pass |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `python3 tools/run_driver_test.py ccd_ptp --hw --device "Sony Alpha A7RIV"` | Pi 5 `~/indigo`, Debian 12 aarch64, sources hash-identical, no tracing | 0 | 16/16 OK, 20:55 UTC |
| `python3 tools/run_driver_test.py ccd_ptp` | repo root, macOS arm64 | 0 | 180/180 OK, 22:00 local |
| `make -C indigo_test test-ccd-ptp-sdk-sanitize test-ccd-ptp-ica-sanitize` | repo root, macOS arm64 | 0 | all pass |
| negative controls (`INDIGO_TEST_CASE_FILTER=<case> make -C indigo_test test-ccd-ptp-sdk`, one fix disabled) | repo root | 2 | listed cases fail |
| `./.factory/scripts/security`, `./.factory/scripts/findings check` | repo root | 0 | pass |
| Pi-only probes and traced runs | Pi | various | diagnosis; not counted |

## Baseline comparison and regressions
Baseline 3.0.0.60: stale 10 s image returned after a reconnect (probe, EXIF). Superseded recorded runs: 20:10 16/15, 20:34 16/15 (test did not restore the shutter), 20:39 16/16 with one empty read. Pre-existing `-Wstringop-overflow` warning at `indigo_ptp.c:1897` unchanged.

## Findings
Defects D-PM1 and D-PM2 fixed (driver REFACTOR.md); one empty read at 20:39 UTC not reproduced, mitigated, cause unconfirmed; no finding record created.

## Manual checks and limitations
Physical hot-plug not run; PriorityMode effect unmeasured (owner decision); ILCE-7RM2 and X-T1 not rerun.

## Verification verdict rationale
All acceptance criteria verified on the final candidate.

## Register reconciliation
No finding records.
