# Verification

Reviewed base: aa8faa422 (local master), task branch with scoped dirty source/test/docs changes. Latest local master was confirmed unchanged and `git merge --ff-only master` returned Already up to date before final verification. Root performed implementation and these verification commands; independent quality/security review is assigned separately. CWD for commands below: isolated task root `/private/tmp/indigo-pentax-fix`; host macOS arm64, clang, Python 3, local copied library/header prerequisites. No network or hardware execution.

## Acceptance

- AC-1 passes: failed open retry asserts zero invalid handle calls/close and successful recovery.
- AC-2 passes: immediate/delayed readiness and camera error preserved; both never-ready modes fail within the fake monotonic deadline rather than forced transport removal. Polling budget is five seconds plus at most one bounded in-flight SCSI transaction, not a hard five-second teardown guarantee.
- AC-3 passes: all response entry points reject zero/short/oversize lengths before read; 65,535 bytes remains accepted by general state. All five field extents tested for every table model before any state update; exact-layout successes for every model/endian; short/error CBW/data/CSW and invalid signature/tag/residue rejected. Private bounded storage replaces camera-sized stack allocations. Existing ushort read wire format is preserved.
- AC-4 passes: repeated unplug fixture tracks one private free, one unref and detach-before-free, even when detach clears device->private_data.
- AC-5 passes: original success traces retained exactly, version increased from 3.0.0.1 to 3.0.0.2; Makefile/Xcode registrations and testing documentation match delivered suite.

- AC-6 passes: hotplug descriptor names verifies absent indexes and failed reads use PENTAX Camera; full non-NUL descriptor buffers produce exactly 31 manufacturer bytes plus space plus 31 product bytes, with balanced ownership. New defect source-confirmed during independent review; no original unsafe runtime reproduction claimed.

## Executed commands and results

- Baseline `make -C indigo_drivers/ccd_pentax -f ../../Makefile.drv all`: pass after creating missing build output directories. Original-driver `make -C indigo_test build/integration/test_ccd_pentax_usb` and `indigo_test/build/integration/test_ccd_pentax_usb --baseline`: 6/11 passed, five dedicated expected failure cases, retained in baseline.log. No existing Pentax suite preceded this task.
- Repaired production build, same make command: pass, universal arm64/x86_64; native-build.log.
- `python3 tools/run_driver_test.py ccd_pentax`: pass, 17/17 registered fake USB cases; recorded-run.log. Runner records README Testing and regenerates TEST_SUMMARY.md.
- `make -C indigo_test build/integration/test_ccd_pentax_usb_sanitize` then `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 indigo_test/build/integration/test_ccd_pentax_usb_sanitize`: pass, 17/17, ASan/UBSan, sanitizer.log. Initial detect_leaks=1 invocation failed because LeakSanitizer is unsupported on macOS; no leak-scanner pass claimed. Seven warnings are existing sprintf deprecations.
- `./.factory/scripts/check`: pass (diff whitespace, finding integrity, decision index, project boundaries), check.log; refreshed after five finding closures; 27 active / 190 archived records valid.
- `./.factory/scripts/security`: pass (configured local boundary validator), security-check.log.
- `./.factory/scripts/test`: pass, 190 tests in 124.940 seconds, factory-tests.log.

- `plutil -lint indigo.xcodeproj/project.pbxproj`: pass; Xcode project syntax validated, no application build or Factory reference added.

The hotplug test extension initially failed compilation due to a nonexistent copy helper; it was corrected to fixed-size memcpy and rebuilt successfully. An old binary run after the failed compile was not counted as the 17-case evidence. Only the separate latest recorded run and sanitizer logs support final counts.

## Limits

Helper-level production code tests exercise fake USB, real property allocation/update calls and unplug logic; they do not certify public-bus concurrency, real asynchronous connection/teardown, full camera capture or physical transport recovery. No hardware tests (0/0), no Linux/Windows runtime or x86_64 runtime test. No new dependency or generator change. Copied prebuilt local libraries are test prerequisites, not a from-scratch core build. No full-repository audit claim; scoped owner-approved deferral remains active.

Verdict: pass
