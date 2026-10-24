# Verification

Verdict: pass

Root implemented and verified the dirty candidate based on afdf183985d8337d51b7f1d0d57ffd58859e9452; this verification is not independent. Working directory: /Users/polakovic/Development/indigo, macOS arm64.

AC-1: baseline move-inventory.json contains each of eight source SHA-256 hashes and permission modes. A Python comparison confirmed the new folder has exactly those eight files, identical bytes/modes, and indigo_tests is absent. Seven `bash -n` invocations passed, one per relocated shell file. No source behavior or license header was changed.

AC-2: `python3 -B indigo_test/unit/test_gps_compliance_script.py` passed 9 tests in 35.223 seconds using synthetic fake I/O (retained log: gps-regression.log). Direct consumers now use the new path. `plutil -lint indigo.xcodeproj/project.pbxproj` passed; the existing shell group and all file objects were preserved, reparented under indigo_test and renamed shell_compliance. Current Markdown paths were checked against the relocated files.

AC-3: no default Makefile target changed. Root/scoped instructions, project context and archive exclusion retain the opt-in shell boundary at its new location, including an archive-exclusion fixture. Historical ADR and commit-pinned finding paths retain their original evidence; the old archive exclusion is retained for historical input compatibility.

`./.factory/scripts/check` and `./.factory/scripts/security` passed. Full configured Factory tests are recorded separately below after completion. No hardware, server, native application build or broad driver test was needed or run for byte-identical relocation. No new tests merely mirror the move; existing GPS and archival regressions exercise its actual consumers.

Final configured Factory result: `./.factory/scripts/test` passed 188/188 tests in 104.922 seconds (factory-test.log). `git fetch origin` and `git merge --ff-only origin/master` reported already up to date at afdf183985d8337d51b7f1d0d57ffd58859e9452 before final closure. Independent review also checked Xcode parentage and all eight resolved file references. All AC-1 through AC-3 pass.
