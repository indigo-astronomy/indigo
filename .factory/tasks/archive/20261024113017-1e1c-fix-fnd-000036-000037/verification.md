# Verification

Verdict: pass

Reviewed candidate: base master `25c92ed424f747af067a2009981915c7f65b186a` plus this task's uncommitted diff. Root verifier also implemented production changes; delegated test author and independent quality/security reviewer provide separate-context checks. Exact final staged content identities are retained in process.json. All commands below ran from `/Users/polakovic/Development/indigo` on Darwin arm64, Python 3.9.6 and Apple clang 21.0.0. No hardware or network used.

## Acceptance evidence

| Criterion | Result and evidence |
| --- | --- |
| AC-001 | Pass: ordinary names, leading/trailing item/property whitespace, quotes and escaped quotes; fitting and oversized names/values; exactly 128 and excess items, malformed/empty fields. Checked spans reject before copying; item count is tested before indexing. |
| AC-002 | Pass: real CLI main accepts fitting hostname and ports 1/65535 through --help and rejects oversized/invalid endpoints before connection. Source review verifies bounded copied hostname and preserved default port. |
| AC-003 | Pass: supported signatures, complete header/payload, positive dimensions, size_t overflow guards, strict numeric ROI and subtraction-based containment checks. Bounded malformed fixtures fail with positive exit rather than crash. |
| AC-004 | Pass: exact cropped pixels for all four types, full frame and last pixel, trailing metadata accepted. Separate-TU fault injection covers both allocations, header/payload read/write and input/output close failures; output-open errors propagate. |

Detailed fixtures, command structure, sanitizer limits and baseline evidence: [test-evidence.md](test-evidence.md). Independent final-diff and security results: [quality-review.md](quality-review.md), [security.md](security.md).

## Executed checks

- `make -C indigo_test test-tool-input-safety`: pass, 5 C parser cases and 6 Python RAW cases with boundary/failure subcases. Temporary binaries/fixtures self-clean. The test agent and independent reviewer also ran `SANITIZERS=address,undefined make -C indigo_test test-tool-input-safety` / equivalent direct Python invocation: final source passes all 11 cases without sanitizer reports.
- `make -C indigo_tools /Users/polakovic/Development/indigo/build/bin/indigo_prop_tool /Users/polakovic/Development/indigo/build/bin/indigo_raw_crop`: pass, configured universal arm64+x86_64 macOS build. Earlier relative target invocation did no work and is not build evidence.
- `./.factory/scripts/check`: pass; diff whitespace, finding integrity, decision index and project boundaries.
- `./.factory/scripts/test`: pass, 188 Factory regression tests in 110.091 seconds.
- `./.factory/scripts/security`: pass, configured local Factory boundary validation; does not replace source security review.
- `plutil -lint indigo.xcodeproj/project.pbxproj`: pass.
- `git rev-parse master origin/master`: both remain `25c92ed424f747af067a2009981915c7f65b186a`, same included local target; no remote fetch/publication authorization inferred.

## Baseline and limitations

Original RAW crop was extracted from the base revision and compiled with `cc -Iindigo_libs <temporary>/raw_crop.c -o <temporary>/crop`. Four-byte 2x2 RAW1 fixtures showed valid crop exit 0 and incorrectly returned 0 for zero width, truncated pixel payload and invalid signature. Additional safe baseline cases in test-evidence.md show ignored trailing ROI and output-open failure. Dangerous original-parser overflow cases were not executed; source inspection established unbounded/equal-capacity sscanf widths and overlapping source/destination scanning. Repaired candidate receives bounded sanitizer tests.

Existing unrelated property display `sprintf` deprecation warnings remain. Prebuilt client library is linked but not rebuilt under sanitizers. No Linux, Windows, hardware, full application suite or whole-repository audit certification is claimed. RAW file-size preflight uses portable C fseek/ftell and safely rejects files whose offsets the platform cannot represent (notably long-limited platforms). Output errors return failure and can leave a partial output file. No new dependency, driver/property change or public API change.

Final committed candidate must pass the committed-range process check and required configured checks before being reported ready; integration/publication remain pending under owner authorization.

Final closure metadata inspection: finding lifecycle helpers moved only FND-000036/FND-000037 into the closed registry and retained their Xcode references. Findings validation, project lint and final diff whitespace checks pass. `make -C indigo_test test-clean` removed generated test artifacts. No task evidence is pruned; archive retains reports.
