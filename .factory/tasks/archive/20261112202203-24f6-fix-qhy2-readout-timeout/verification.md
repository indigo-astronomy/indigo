# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: coordinator (implementer)
Date: 2026-11-12
Revision and working-tree state: branch `task/20261112202203-24f6-fix-qhy2-readout-timeout` at master `e0280e650`, with the uncommitted diff of three product files
Review independence: implementer; the diff was also reviewed in a fresh context (quality-review.md)

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `slow readout timeout` prints a frame timeout of 10100 ms for 0.1 s and 11000 ms for 1 s; `.driver` computes `fmin(ceil(duration * 1000) + 10000, UINT32_MAX)` |
| AC-002 | pass | `slow readout timeout` with the fix: 2 blobs, 0 bad payloads (`verify_raw` compares every byte with the SDK pattern) |
| AC-003 | pass | the same case with the HEAD `.cpp`: both frames are zeros, "expected 0, got 2" bad payloads, FAIL |
| AC-004 | pass | full `test_ccd_qhy2_sdk` 43/43, full `test_ccd_qhy_sdk` 39/39 |
| AC-005 | pass | `.driver` `version = 47` (was 46); `.cpp` `DRIVER_VERSION 0x0300002F` (was `0x0300002E`); the generator output is byte-identical to a scratch regeneration (quality-review.md) |

## Executed checks
All commands run on macOS 27.0 arm64 with Apple clang (Xcode).

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make build/integration/test_ccd_qhy2_sdk build/integration/test_ccd_qhy_sdk` (new test, unfixed driver) | `indigo_test` | 0 | built |
| `./build/integration/test_ccd_qhy2_sdk "slow readout"` (first test version, unfixed driver) | `indigo_test` | 1 | FAIL, frame timeout 1000 ms for both exposures |
| `make -C indigo_drivers/ccd_qhy2 -f ../../Makefile.drv` (fixed `.driver`, regenerated `.cpp`) | repository root | 0 | built; only the existing libqhyccd deployment-target linker warnings |
| `./build/integration/test_ccd_qhy2_sdk "slow readout"` (final test, HEAD `.cpp` temporarily restored, then the fixed `.cpp` put back) | `indigo_test` | 1 | FAIL, 2 bad payloads, frame timeout 1000 ms |
| `./build/integration/test_ccd_qhy2_sdk "slow readout"` (final test, fixed driver) | `indigo_test` | 0 | PASS, 10100 / 11000 ms |
| `./build/integration/test_ccd_qhy2_sdk` (final candidate) | `indigo_test` | 0 | 43 pass, 0 fail, 45 s |
| `./build/integration/test_ccd_qhy_sdk` (final candidate, legacy driver) | `indigo_test` | 0 | 39 pass, 0 fail, 39 s |
| `./.factory/scripts/findings check` | repository root | 0 | "Findings valid: 25 active, 292 archived; task evidence checks passed." |

"Acquisition setup failed" lines in the qhy2 log come from the existing injected-failure cases.

## Baseline comparison and regressions
- Baseline: master's qhy2 suite has 42 cases and the legacy suite 39. This was not rerun before the change; the counts come from the case list.
- Final: qhy2 has 43 cases (one new) and the legacy suite 39. All pass.

## Findings
| Defect | Impact | Root cause | Fix | Regression test |
| --- | --- | --- | --- | --- |
| D1 | Single frames of cameras whose exposure plus readout exceeds 1 s are empty (all zeros); reported for a QHY168C in indigo#783 | fixed 1000 ms `SetQHYCCDSingleFrameTimeOut()`, introduced by the generator migration `3768cd34f` | exposure + 10 s, capped at `UINT32_MAX` | `slow readout timeout` |

Hardware observation: indigo#783 (QHY168C, all-zero frames for 0.1–1 s exposures while EZCAP works). It is reproduced in the fake SDK by the frame timeout model. There are no central FND records: the defect is fixed in this task.

## Manual checks and limitations
- No hardware is available. The real SDK's timeout behaviour and the QHY168C readout time are inferred, not measured; confirmation needs the reporter's test of a build.
- No recorded `tools/run_driver_test.py` run was requested, so the driver README test record is unchanged.

Simulated test totals: 43/43 qhy2 fake SDK and 39/39 legacy qhy fake SDK. Hardware: 0 runs.
