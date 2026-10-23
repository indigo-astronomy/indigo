# Verification

Verdict: pass

Final source reviewed on included master c5a8b69ab55098f93e723f3d6ffc2a6cf5cae441 plus the task diff; exact source identities are recorded in process.json. Post-commit gates must pass before integration. Same agent implemented and verified in separate passes; no independent reviewer.

## Environment and baseline

Working directory: /private/tmp/indigo-20261023181210-45bf-fix-raw-selection; base 3b48146ca79b4eb314460d4f9c3728b11df9990c plus task diff. macOS arm64; native core compiler /usr/bin/clang, universal core build; sanitizer runs arm64. No hardware or network tests.

`make Makefile.inc` succeeded. `make -C indigo_libs -j4 all` initially failed in bundled libusb linking for x86_64 (undefined internal usbi symbols). No source fix was attempted outside scope. Existing third-party headers and libusb-1.0.dylib, libhidapi.a, libftd2xx.a, libjpeg.a, libtiff.a, libtiffxx.a, libraw.a were copied from the primary checkout's ignored build directory into this isolated worktree. Only those dependency artifacts were reused; production INDIGO core sources and changed helpers were compiled here. Repeated `make -C indigo_libs -j4 all` succeeded. This is not a fresh-dependency-build certification or Linux/Windows runtime verification.

`make -C indigo_test build/unit/test_raw_image` succeeded on baseline source. Case-filtered executions (see baseline.log) reproduced incomplete-ROI, nonpositive-radius and fractional-variance failures. High-contrast mono16 happened to pass under optimized native compilation; instrumented baseline failed explicitly with a 2.41585e+09-to-int conversion at indigo_raw_utils.c:512 (baseline-sanitize.log). The baseline preservation fixture originally expected integer centroid 4; the documented existing pixel-center formula produces 4.5. The expectation was corrected before implementation and rechecked separately; production centroid behavior is unchanged.

The baseline sanitizer compiled indigo_raw_utils.c separately with `clang -g -O1 -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -DINDIGO_MACOS -Duint=unsigned -Iindigo_libs -Ibuild/include -c indigo_libs/indigo_raw_utils.c -o /private/tmp/indigo-006-sanitize/raw.o`; linked the test source plus that object before build/lib/libindigo.a and existing static codec libraries, USB/HID/system frameworks. `INDIGO_TEST_CASE_FILTER=psf_high_contrast UBSAN_OPTIONS=halt_on_error=1 /private/tmp/indigo-006-sanitize/test_raw` failed at the recorded UBSan error.

## Acceptance and developer checks

AC-001: pass; rejected incomplete ROIs in all six formats, nonpositive/extreme radii, invalid dimensions, NaN/infinite/out-of-range centers, missing pointers, and unchanged output sentinels. Invalid-ROI tests use a one-byte dummy buffer so sanitizer execution verifies rejection precedes reads.
AC-002: pass; complete ROIs touching all four edges in every format, interior star centroid/PSF, no-output PSF shortcut.
AC-003: pass; mono16 extreme border and mono8 fractional variance fixtures give the specified finite diameter/peak results.

After implementation, `make -C indigo_libs -j4 all`, `make -C indigo_test build/unit/test_raw_image`, and `indigo_test/build/unit/test_raw_image` passed, 15/15 cases. `make -C indigo_test test-raw-image-sanitize` passed 15/15 under ASan/UBSan including float-cast-overflow; the target separately compiles production RAW helpers and links them before the static core library. Native full-suite execution, hardware and unrelated drivers are outside this bounded RAW-helper verification.

`./.factory/scripts/check` passed all four configured commands (diff whitespace, findings, decision index, project boundaries). `./.factory/scripts/security` passed local Factory boundaries. `plutil -lint indigo.xcodeproj/project.pbxproj` passed before task references are added. Factory regression suite and final post-synchronization checks pending.

`./.factory/scripts/test` passed 175 Factory regression tests in 109.904s. This does not certify unrelated application drivers.

`make -C indigo_test test-clean` passed after preliminary checks; test binaries removed as required. Final target verification will rebuild them.

## Final synchronized verification

Coordinator supplied c5a8b69ab55098f93e723f3d6ffc2a6cf5cae441 and reserved the next readiness-based integration slot for 006. The isolated task branch fast-forwarded to that exact target; no primary-checkout/master writes. The existing upstream imager deferral and shared Makefile target were preserved explicitly. The original waiting-for-026 gate is superseded by the owner's readiness instruction.

All following commands executed from the working directory above on the final synchronized source and returned 0:

- `make -C indigo_libs -j4 all`: core static/shared libraries built for arm64/x86_64, using the previously documented third-party artifacts.
- `make -C indigo_test build/unit/test_raw_image` then `indigo_test/build/unit/test_raw_image`: native 15/15.
- `make -C indigo_test test-raw-image-sanitize`: arm64 ASan/UBSan/float-cast-overflow 15/15, no diagnostics.
- `make -C indigo_test test-imager-download`: inherited regression target preserved, 18/18.
- `./.factory/scripts/test`: all 175 Factory regressions passed on the synchronized target.
- `./.factory/scripts/check` and `./.factory/scripts/security`: all configured commands passed, including findings integrity, decision index and project boundaries.
- `plutil -lint indigo.xcodeproj/project.pbxproj`: valid after task/completion/closed-finding navigation additions.

Separate same-agent final quality/security passes inspected the actual synchronized diff, all new tests and source contexts, maintenance authorization, finding closure and Xcode paths. Finding 006 retains origin and repair evidence in its closed record. The full repository audit remains due under the explicit scoped owner deferral. No owner risk waiver is used.

Archival, staged-content recording and committed-range process validation are required delivery gates. Their actual results and final SHA are reported externally after commit to avoid self-referential evidence. Committed-candidate RAW/sanitizer and configured checks will be rerun before handing the exact commit to the coordinator. No hardware, driver suite, full native unit-suite or Windows/Linux runtime pass is claimed.

Archive preview and archival succeeded (0 incoming Markdown links needed rewriting). Post-archive configured check/security and findings integrity passed. Parsed Xcode navigation resolves all 14 newly referenced files, including retained baseline logs, completion and closed finding. The coordinator also reported a supplemental read-only review of the synchronized source with no new issue; this does not change the same-agent formal review limitation.
