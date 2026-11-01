# Detector verification

Verdict: pass

This verdict means the requested diagnosis was completed, not that the defective functions are safe or repaired. Reviewed source: 56ee48c9ea3c1155205798ecb2ce04f2a5899488, initially clean master; source identical to e5e77e58edd20f102577a8f931651f802bdfc612. No production changes. Same agent performed analysis and verification.

## Source and reachability (AC-001, AC-005)

`git diff e5e77e58edd20f102577a8f931651f802bdfc612 HEAD -- indigo_libs/indigo_raw_utils.c` was empty. SHA-256 of both this file and `/Users/polakovic/Development/IndigoApps6/INDIGO/src/indigo_raw_utils.c`: `106370f18ce08c7c3b1444762e6e673c8558ddccb72f16e725735305dcd1a0ba`.

Solver call sites are lines 823 and 826 of indigo_drivers/agent_solver/indigo_solver.c, with radius 16. This is static reachability only; no end-to-end agent exploit/runtime validation was performed.

## Direct-source sanitizer run (AC-002, AC-003)

Working directory: /Users/polakovic/Development/indigo. Apple clang 21.0.0 (clang-2100.3.34.2), macOS host. Exact successful build/run commands and outputs are retained in sanitizer-verification.log, generated with `python3 .factory/tasks/20261101185459-950f-verify-star-detectors/run-verification.py` (exit 0). Both full translation units were compiled directly with `-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all`. Existing build/lib/libindigo.dylib supplies dependencies and was not rebuilt/instrumented; tested detector bodies are compiled from the source under examination. ASAN_OPTIONS=symbolize=0 avoids a host symbolizer failure; UBSan retains source line locations. Initial linking with build/ failed because libraries are in build/lib/; corrected before successful runs.

- Current fast, 144 x 3 MONO16, radius 3: ASan heap-buffer-overflow, READ of size 2, 2 bytes before 864-byte buffer.
- Current precise, same edge input: identical class and offset of ASan failure.
- Current fast, 64 x 64 constant MONO16, radius 32768: UBSan signed overflow 65537 * 65537 at line 3275.
- Current precise, same radius input: UBSan signed overflow at line 2859.
- Precise before optimization (21d2af5c5^): same empty radius input exits 0, result=0 count=0. The optimization commit 21d2af5c55184ed01dd2db59976a4b5384d95307 moved allocation outside the candidate branch. This is a confirmed new regression for empty images, even though candidate-path overflow already existed.

Original edge/overflow logs are also retained individually. Fixture extends the supplied reproduction only to select precise for the edge case.

## Median and precise compatibility (AC-004)

compatibility-fixture.c includes the full source, allowing direct testing of the actual static median implementation. 8,192 comparisons (lengths 1–1024, eight patterns including equal values, extrema, sorted/reverse, duplicate/random and alternating values) all equal sorted[count / 2]. No empty median input is asserted supported.

288 precise comparisons: 12 deterministic synthetic noisy star fields x 6 formats (MONO8, MONO16, RGB24, RGBA32, ABGR32, RGB48) x radii 0/1/3/16 at 96 x 96. Every result, count, coordinate, luminance, normalized distance and flag printed with 17-digit precision matches the pre-optimization source byte for byte. Nonempty detection is asserted. Both executables ran without sanitizer diagnostics. This is bounded empirical evidence plus inspection of the median contract, not proof over all images.

## Disposition and limits

FND-20261101185459-950f-verify-star-detectors-001: neighbor overread in both detectors; open, high.
FND-20261101185459-950f-verify-star-detectors-002: unchecked radius arithmetic in both detectors; open, medium.

Repair must enforce a valid neighbor scan range, validate geometry/radius and checked allocation arithmetic while preserving radius 0. No repair was requested/applied. A repaired-source matrix of short/narrow/tiny images, all formats, invalid/large radii and ordinary detection remains the repair task's acceptance gate. This verification does not claim that matrix passed or that agent exploitability was demonstrated.

## Repository checks

Configured check, test and security groups passed and are recorded in factory-*.log; the Factory unit suite ran 190 tests in 106.818 seconds. No broad native build, off-limits driver or shell-compliance suite was run. Repository audit remains overdue under the owner's verification-only deferral; this task does not reset it. Local master was not updated from remote, and no integration/publication was requested; all claims are tied to the pinned local source.
