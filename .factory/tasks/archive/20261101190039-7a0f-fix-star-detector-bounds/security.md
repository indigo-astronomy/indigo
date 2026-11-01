# Independent security review

Verdict: pass

Reviewer: Codex subagent review_detector_fix, independent of the parent source/test author. Date: 2026-11-01. Base: 7ea8e5e5346dae1c773377110a36732d133527d0 with the scoped repair diff. Reviewed detector source SHA-256: 7a35192c266c72bdbaf568f8d68671c642b9c423364bc5f141a424c114445648. This is a local memory-safety repair review, not a whole-library security certification.

Both entry points validate radius and geometry before data conversion/allocation. Radius is restricted to 0..128, matching the pre-existing centroid maximum and bounding squared-distance and widened-fallback arithmetic. Negative/extreme radii fail without inspecting the image. Positive geometry is bounded before width*height, 3/4*size or grid-coordinate products are evaluated; size_t allocation guards account for luminance storage, initial candidate storage, candidate doubling and square scratch storage. The scratch side is computed in size_t, and checked division precedes side*side*sizeof(uint16_t). With clip_edge at least one, every scanned candidate has all eight neighbors inside the luminance buffer; images without an interior produce no candidates.

Reinspection resolved the reviewer-noted 32-bit candidate-growth concern through an additional pixels <= SIZE_MAX / sizeof(local_maximum) / 2 guard. Actual 32-bit execution was unavailable; the bound was checked statically. No authentication, network access, secrets, external dependencies, driver behavior or public ABI changes were introduced.

Independent execution from /Users/polakovic/Development/indigo used Apple clang on macOS:

`clang -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -I indigo_libs indigo_test/unit/test_star_detection.c indigo_libs/indigo_raw_utils.c -L build/lib -lindigo -Wl,-rpath,/Users/polakovic/Development/indigo/build/lib -o /tmp/indigo-detector-independent-review`

`ASAN_OPTIONS=symbolize=0 /tmp/indigo-detector-independent-review`

Final result: exit 0, all four groups pass, including six formats, both detectors, tiny/short/narrow images, invalid/huge radius and geometry, radius zero, ordinary stars and radius 128 detection. No sanitizer diagnostics. I also independently ran verify-sanitizers.py before the final additional radius-128 group; its original reproductions, 8,192 median checks and 288 historical precise output comparisons passed. The final additional group was independently compiled/executed as above. Dependency library build/lib/libindigo.dylib was not rebuilt/instrumented; the entire raw-utils translation unit under review was compiled directly with both sanitizers.

`./.factory/scripts/security` passed independently (Factory local security boundaries valid). Verification.md retains coordinator acceptance evidence and logs. Canonical findings FND-20261101185459-950f-verify-star-detectors-001 and -002 have verified repairs and are suitable for coordinator closure; this report does not itself modify registry state or accept residual risk. No remaining blocking security finding was identified in this repair. End-to-end agent reachability, Linux/Windows/32-bit runtime and arbitrary unrelated RAW helpers remain outside this review.
