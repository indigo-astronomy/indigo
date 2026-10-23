# FTDI build verification

Verdict: pass

Scope: task 002-fix-ftdi-build, final working-tree content against e5f25534910b7f51d3e1e87daace13dcd9820d90. Verifier is the same agent in a separate explicitly loaded role. macOS arm64, Python 3.9.6, Apple Git 2.54.0; configured universal library build targets x86_64/arm64. Final fetched master remains eba315b7041b1a19f64a1ad7864dc1a6c0019ea8, already an ancestor of the adopted task base. No integration/merge/push performed.

## Baseline and iterations
FND-000044: original make -C indigo_libs all outside sandbox exited 2 at normal hdiutil detach, vetoed by Xcode. The copied final target matched the unprocessed vendor archive, retaining all nine embedded libusb members. The diagnostic removed that incomplete output and detached the image.
Regression fixtures first failed against the absent helper/legacy recipe. Test setup initially confused Apple's xcrun_db cache with helper output; using direct Python/make executable paths fixes isolation without weakening assertions. The Makefile test refuses to invoke the legacy global-mount recipe. The one baseline synthetic legacy directory was verified as fixture-only and removed without touching a real mount.
The first real implementation build exposed local lipo rejecting multi-architecture -verify_arch argv. Changed verification to one invocation per architecture and strengthened the fake command contract. That failure also exercised real normal-detach refusal followed by successful force-detach cleanup, with no final archive left. No test was weakened to hide this error.

## Exact final checks
All commands from the repository root unless -C selects the subdirectory. Raw diagnostic logs remain private temporary artifacts; relevant outcomes are retained here.

| Command | Exit | Outcome |
| --- | --- | --- |
| python3 -B -m unittest discover -s indigo_test/buildsystem -p test_ftdi_build.py | 0 | 7 tests, 8.328 s; processing-failure test includes 9 subcases |
| make -C indigo_test test-buildsystem | 0 | registered target, 7 tests, 6.249 s |
| sh -n tools/build_ftdi_macos.sh | 0 | POSIX shell syntax accepted |
| make -C indigo_libs all (outside sandbox) | 0 | full library-only build succeeds; final-source repeat also exit 0 |
| ./.factory/scripts/test | 0 | 144 Factory regression cases, 104.291 s |
| ./.factory/scripts/check | 0 | static/diff/findings/ADR/project checks pass |
| ./.factory/scripts/security | 0 | local workflow security boundary checks pass |
| plutil -lint indigo.xcodeproj/project.pbxproj | 0 | valid Xcode structure; new source/test/task/finding paths registered |

Artifact validation: FTDI retains x86_64/arm64/arm64e, 22 members per slice, none of the nine embedded libusb objects in any slice. libindigo.a/.dylib, libindigo_client.a/.dylib and libindigocat.a/.dylib all exist and contain x86_64/arm64. See artifact-validation.json. hdiutil info verified no FTDI image remains mounted. The final successful build detached normally; the real earlier failed-build cleanup and fixtures separately verify the force fallback.

## Acceptance
AC-001 passes: normal refusal retries force only on the unique private read-only mount; both-refused case retains the workspace with an error, without publication. AC-002 passes: all slices stripped/indexed/recombined and individually verified before atomic rename; failures leave no new target. AC-003 passes: first/later slice, ar, ranlib, combine, verify, headers, termination and partial-attach failures clean staging and owned mounts; existing target retained. Paths with spaces are covered. AC-004 passes: library build succeeds outside sandbox, no further compiler/linker failure remains.

No native driver/hardware run was requested or executed. Linux recipe was left unchanged; no claim of Linux/Windows execution. No application/vendor-source/README/protected-driver changes or new dependencies. This is build validation, not device-behavior or full-repository audit coverage. Owner explicitly prioritized this repair; full audit remains due afterward.

## Delivery and retention
After task archival rerun integrity/whitespace/Xcode checks, capture exact staged content and commit locally. Committed candidate process validation and final commit identity are reported externally, avoiding self-referential hashes. Source or target changes require affected checks refreshed.
