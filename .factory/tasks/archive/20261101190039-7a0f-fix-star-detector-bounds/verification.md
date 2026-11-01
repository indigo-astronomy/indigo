# Star detector repair verification

Verdict: pass

Reviewed working-tree repair against base 7ea8e5e5346dae1c773377110a36732d133527d0, retaining the preceding verification task and its original reproductions. Local master remains 56ee48c9ea3c1155205798ecb2ce04f2a5899488 and is included in this branch. Source author and parent verifier are the same agent; independent reviewer review_detector_fix separately inspects and executes checks (quality-review.md, security.md). No remote synchronization, integration or publication is claimed.

## Behavior and acceptance

AC-001: both detectors clamp the existing height-based scan margin to at least one. Every scanned x/y is strictly inside the corresponding image dimension. Positive tiny/narrow images with no interior scan return zero stars, without touching neighbor offsets.

AC-002: common validation rejects negative or >128 radius (matching the existing MAX_RADIUS centroid limit), nonpositive dimensions/limit and dimensions exceeding the existing signed int channel/grid indexing range. Candidate allocation/growth representability is checked for size_t, including 32-bit capacity doubling; scratch side and byte products are checked size_t arithmetic. Invalid calls return INDIGO_FAILED before input reads or allocations and preserve the count output, matching prior failure conventions. Public header documents the supported range. Radius 0 remains accepted.

AC-003: direct-source ASan/UBSan matrix passes. The new native test is registered in UNIT_TESTS and compiles indigo_raw_utils.c directly so instrumented runs execute the changed bodies, rather than the existing shared-library detector. Existing build/lib/libindigo.dylib supplies dependencies and is not itself rebuilt/instrumented; sanitizer claims cover the directly compiled translation unit. No mock detector is used.

AC-004: the actual static quickselect implementation equals a sorted-median reference for 8,192 inputs. 288 ordinary precise outputs equal the source before optimization (21d2af5c5^) byte for byte: 12 deterministic noisy star fields x all six formats x radii 0/1/3/16. Compared output contains result/count, positions, luminance, distance and both flags at 17-digit precision. No median code changed.

AC-005: both canonical source findings are verified fixed; their registry closure follows independent review. No risk acceptance is needed or claimed.

## Exact commands and results

Working directory /Users/polakovic/Development/indigo; Apple clang 21.0.0 (clang-2100.3.34.2), native arm64 macOS runtime. `python3 .factory/tasks/20261101190039-7a0f-fix-star-detector-bounds/verify-sanitizers.py` exits 0. Full compiler argv, executable argv and outputs are retained in sanitizer-verification.log. Script resolves its root after archival as well. All compiled translation units use `-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all`; ASAN_OPTIONS=symbolize=0 avoids the prior host symbolizer limitation. No sanitizer diagnostic occurred.

- 3,960 small-image calls: 11 geometries (1x1, 1x144, 144x1, 2x144, 144x2, 3x3, 3x144, 144x3, 4x4, 8x8, 64x64), six formats (MONO8/16, RGB24/48, RGBA32, ABGR32), constant/corner/interior isolated bright pixel patterns, radii 0/1/3/16/128, both detectors. All return success zero stars for these constant/hot-pixel fields.
- 192 invalid radius/geometry calls across both detectors and six formats: INT_MIN/-1/129/32768/(INT_MAX-1)/2/INT_MAX radii, zero/negative dimensions, INT_MAX axes, 46341-square and 32768-square overflow geometries, grid-product limit violations. One-pixel inputs ensure sanitizer catches any mistaken pre-validation access. Additional NULL pointers and zero star-limit cases fail cleanly.
- 60 ordinary Gaussian field calls across both detectors/all formats: radii 0/1 return success zero by existing rejection rules; radii 3/16 return one star on 96x96; maximum supported radius128 returns one star on 272x272. Centroids match pixel-center positions 48.5 / 136.5.
- All four exact original reproduction modes exit cleanly: fast/precise edge input result=0 count=0; fast/precise radius32768 input result=1 (INDIGO_FAILED), count remains -1.
- 8,192 median reference cases and 288 historical precise comparisons pass, as above.

`make -C indigo_test build/unit/test_star_detection` and `indigo_test/build/unit/test_star_detection` verify normal Makefile registration; make.log records the universal x86_64/arm64 build and native execution. Final rebuilt fixture has four passing groups. `make -C indigo_test test-clean` removes test artifacts afterward. No Windows/Linux/32-bit runtime or end-to-end Solver-agent run was available/performed; integer and size bounds on those targets were reviewed statically.

## New test expectation corrections

Initial new Gaussian assertions expected a radius1 detection and integer-centered coordinates. Pre-optimization source reproduced the radius1 zero result (initial-normal-expectation.log). Existing upper median of a 3x3 radius1 neighborhood permits at most four neighbors strictly above it, fewer than the required six. The digest returns pixel-center coordinates with a half-pixel offset (initial-centroid-expectation.log records the initial mismatch). The corrected ordinary case was run against pre-optimization source and passed (normal-baseline.log); reviewer independently confirmed both semantics. These corrections affect new tests only; no existing assertion or production behavior was weakened.

## Repository gates and scope

Configured Factory check/test/security groups passed; outcomes are retained in factory-*.log (190 Factory unit tests). No driver, generator, properties, Xcode project, mirror, dependency or ABI change. Required final committed-range process check follows the final commit and is reported externally. The owner's explicit repair-only audit deferral remains in effect through this task; the repository audit remains due afterward and this diff review does not reset its checkpoint.
