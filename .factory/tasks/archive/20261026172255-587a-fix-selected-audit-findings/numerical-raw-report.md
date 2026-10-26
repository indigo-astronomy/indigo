# Numerical and RAW repair developer report

Status: implemented and locally verified; independent review is pending. Source base: a35823d61, dirty task branch task/20261026172255-587a-fix-selected-audit-findings. Working directory for every command below: /private/tmp/indigo-selected-audit-fixes. Host: macOS arm64, /usr/bin/clang. No external service, hardware or generator change.

## Changes and acceptance

- AC-002 / bc8d-012: minimum Bayer sample steps are 2, preserving the parity of every sampled 2x2 cell. Existing RAW unit file adds 4x4 repeated-tile checks for 8-bit and 16-bit data with distinct levels and near-saturation levels. Constant per-parity tiles must equalize to their overall mean (one integer rounding unit allowed). No gain/clamping behavior changes.
- AC-004 / bc8d-034: reset the longitude and obliquity series accumulators whenever the cached date is recomputed. A test includes production source to compare changing-date results against explicitly fresh internal cache state, at three fixed epochs. No cache threshold/API change or broad astronomical accuracy claim.
- AC-005 / bc8d-033: return LIBRAW_UNSUFFICIENT_MEMORY immediately when libraw_init returns NULL. Applied to both existing wrapper entry points: the second unchecked caller, indigo_dslr_raw_image_info, has the same allocation contract. Tests inject failure in both, assert no open/cleanup calls and empty processed-image output, and verify normal post-init open errors still propagate and run the three cleanup calls. Public API unchanged.
- AC-006 / bc8d-015: convert the 20.4898 arcsecond solar aberration to degrees using 3600, preserving heliocentric radius scaling. Test compares apparent coordinates at J2000 and two other epochs against geometric position plus nutation minus independently expressed arcseconds-to-degrees aberration, using the existing public transform routine. This isolates the unit contract, not broad ephemeris accuracy.

## Commands and evidence

1. `make -C indigo_test test-raw-image-sanitize`
   - Before fix: expected new parity failure, expected 25 got 12; other existing cases passed. See numerical-raw-baseline.log.
   - After fix: all 22 cases passed under address/undefined/float-cast-overflow sanitizers; no diagnostic. The existing heap-accounting check reports unavailable under the sanitizer allocator; it does not certify absence of leaks. See numerical-raw-fixed.log.
2. `/usr/bin/clang -g -arch arm64 -DINDIGO_MACOS -D_GNU_SOURCE -Iindigo_libs -Ibuild/include -O1 -fsanitize=address,undefined -fno-omit-frame-pointer indigo_test/unit/test_indigocat_corrections.c build/lib/libindigocat.a -lm -o indigo_test/build/unit/test_indigocat_corrections`
   followed by `ASAN_OPTIONS=abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 indigo_test/build/unit/test_indigocat_corrections`.
   - Before fix: 2/2 expected failures, history error approximately 1.24e-10 degrees and solar right-ascension mismatch approximately 0.00625 degrees. See numerical-astro-baseline.log.
   - After fix: 2/2 pass with no sanitizer diagnostic. See numerical-astro-fixed.log.
3. `/usr/bin/clang -g -arch arm64 -DINDIGO_MACOS -D_GNU_SOURCE -Iindigo_libs -Ibuild/include -O1 -fsanitize=address,undefined -fno-omit-frame-pointer indigo_test/unit/test_dslr_raw_init.c build/lib/libindigo.a build/lib/libraw.a build/lib/libjpeg.a build/lib/libtiff.a -lz -lc++ -lm -lpthread -lusb-1.0 -lhidapi -Lbuild/lib -framework IOKit -framework CoreFoundation -o indigo_test/build/unit/test_dslr_raw_init`
   followed by `ASAN_OPTIONS=abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 indigo_test/build/unit/test_dslr_raw_init`.
   - Before fix: UBSan member access through NULL in indigo_dslr_raw.c:151, process exited before other cases. See numerical-dslr-baseline.log. Baseline used `ASAN_OPTIONS=abort_on_error=0 UBSAN_OPTIONS=halt_on_error=1`.
   - After fix: 3/3 pass with no sanitizer diagnostic; logs for injected allocation and open failures are expected. See numerical-dslr-fixed.log.
4. `/usr/bin/clang -g -arch arm64 -DINDIGO_MACOS -D_GNU_SOURCE -Iindigo_libs -Ibuild/include -c indigo_libs/indigo_dslr_raw.c -o indigo_test/build/unit/dslr_raw_check.o`: pass, standalone production source compilation.

Sanitizer tests compile changed production implementations directly, so copied prebuilt libindigo/libindigocat archives supply unrelated dependencies and cannot mask these four fixes. Platform execution covers macOS arm64 only; Windows/Linux execution and real RAW files are not claimed.

## Integration handoff

Parent owns Makefile and Xcode changes. Add persistent test files `indigo_test/unit/test_indigocat_corrections.c` and `indigo_test/unit/test_dslr_raw_init.c` to the existing unit-test group; no Factory file belongs in Xcode. Make the new targets part of the unit suite. Dependency rules must include the included production C sources (nutation/sun and DSLR respectively). Astronomy links libindigocat; DSLR needs LibRaw plus existing core/image-library dependencies (the exact sanitizer command above has passed). Existing `test_raw_image.c` is extended, with no new file reference required.

Changed production files: indigo_raw_utils.c, indigo_dslr_raw.c, indigocat_nutation.c, indigocat_sun.c. New test files listed above. No findings registry, Makefile, project file or Git changes made by this developer. No unresolved implementation question. Next: coordinator integrates build entries, rebuilds candidate libraries, runs final affected suites and assigns independent review.
