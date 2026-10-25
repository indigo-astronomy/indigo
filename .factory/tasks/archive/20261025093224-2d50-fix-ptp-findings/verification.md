# Verification report

Verdict: pass
Reviewer: verifier pass by the implementing agent (Claude Code desktop, claude-opus-5-5)
Date: 2026-10-25
Revision and working-tree state: `task/20261025093224-2d50-fix-ptp-findings` at `93bb23d76` plus the uncommitted task diff
Review independence: not independent; the diff review in quality-review.md was done by a fresh read-only subagent.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `device_info_truncated`, `device_info_exact_buffer`, `device_info_short_array`, `device_info_oversized_array` |
| AC-002 | pass | `descriptor_oversized_enum` (1024 numeric values, 16 strings), `descriptor_truncated_enum` (4 of 10) |
| AC-003 | pass | `sony_oversized_extension_info`, `canon_oversized_device_info_ex` (1023 codes kept) |
| AC-004 | pass | `canon_custom_func_ex_bounds`, valid record stored, six malformed records rejected with nothing stored |
| AC-005 | pass | `object_size_mismatch_nikon`, `_sony`, `_fuji`: BLOB equals the object for +4096 and -100 byte metadata, no GetObject for a 20-byte ObjectInfo |
| AC-006 | pass | `canon_long_text_property`: 300-byte value sent as 255 bytes plus terminator (request length 264), short value unchanged |
| AC-007 | pass | recorded run 159/159, sanitizer run 159/159 |

## Executed checks
| Command | Working directory / environment | Exit code | Result |
| --- | --- | --- | --- |
| `make -C indigo_test test-ccd-ptp-sdk` (baseline at 93bb23d76) | repo root, macOS arm64 | 0 | 139/139 fake USB |
| new cases against unchanged 93bb23d76 driver (scratch worktree, `PTP_TEST_FILTER=<case>`) | scratch worktree, same host | various | 11 of 12 failed or crashed, see below |
| `make -C indigo_drivers/ccd_ptp all` | repo root | 0 | production build (ICA .m wrappers and C), no new diagnostics |
| `clang -fsyntax-only -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-deprecated-declarations` on the six changed C files, before and after | repo root | 0 | identical 5 pre-existing conditional-type warnings, none new |
| `make -C indigo_test test-ccd-ptp-sdk-sanitize test-ccd-ptp-ica-sanitize` | repo root, ASan/UBSan | 0 | 152/152 fake USB + 7/7 fake ICA, no sanitizer report (run after the review fixes) |
| `python3 tools/run_driver_test.py ccd_ptp` | repo root | 0 | 159/159 OK, recorded 2026-10-25 10:59 3.0.0.56 mac arm64 fake SDK |
| `./.factory/scripts/findings check` | repo root | 0 | valid |
| `./.factory/scripts/security` | repo root | 0 | Factory local security boundaries valid |

## Baseline comparison and regressions
Pre-fix results of the new cases (unchanged driver at 93bb23d76): `device_info_truncated` passed (USB delivers a zeroed buffer with 1024 bytes padding, so the overread returns zeros); `device_info_short_array` failed (property codes from padding); `device_info_oversized_array` and `canon_oversized_device_info_ex` aborted on the `length < PTP_MAX_ELEMENTS` assertion; `descriptor_oversized_enum` crashed (SIGSEGV); `descriptor_truncated_enum` failed; `sony_oversized_extension_info` aborted (SIGABRT); `canon_custom_func_ex_bounds` crashed (SIGSEGV) at group id 0; `object_size_mismatch_*` published wrong BLOB sizes for +4096 and -100 and crashed on the 20-byte ObjectInfo; `canon_long_text_property` failed (request length from `strlen()` past the copy). `device_info_exact_buffer` uses the new decoder signature and cannot run against the old driver. All 139 pre-existing cases still pass.

## Findings
FND-20261023101753-bc8d-complete-repository-audit-019 and FND-20261024121920-3bd2-repository-audit-026/027/028 closed as fixed; residual FND-20261025093224-2d50-fix-ptp-findings-001 opened. See [findings.md](findings.md).

## Manual checks and limitations
No physical camera. Tests executed on macOS arm64 only; Linux and Windows builds not run. The USB (non-ICA) code path is compiled and exercised by the fake USB suite on macOS.

## Verification verdict rationale
All acceptance criteria are covered by fake USB/ICA cases that pass normally and under ASan/UBSan.

## Register reconciliation
Closed records are in `.factory/findings/archive/`; the new residual record is active.
