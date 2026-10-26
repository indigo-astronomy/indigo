# Verification report

Verdict: pass
Reviewer: root coordinator/verifier, supported by independent in-host scoped reviewers.
Date: 2026-10-26
Revision: base and included local master a35823d61820273d653499f661faab78ae733b10; final uncommitted candidate on task/20261026172255-587a-fix-selected-audit-findings. Master has not advanced. Reports bind final source identities through process.json before commit.
Independence: root implemented portable discovery/SHA and build integration. Other source fixes were implemented by scoped subagents; each production area and test/build integration received a different-agent quality pass. See quality-review.md and security.md; no self-approval or risk acceptance claimed.

## Acceptance

| Criterion | Result | Evidence |
| --- | --- | --- |
| AC-001 | pass | Two synthetic lifecycle cases: one/two original slaves, two clones, three cycles each; base/master identity, enumeration, shared data ownership, connected removal rejection, cleanup/recreate. Both fail against original source and pass normal/ASan/UBSan. lifecycle-report.md; integrated-regressions.log. |
| AC-002 | pass | 4x4 repeated Bayer tiles for 8/16-bit samples and near-saturation levels; original estimator fails, fixed RAW suite 22 cases passes with sanitizers. numerical-raw-report.md; numerical-raw-{baseline,fixed}.log; unit-complete.log. |
| AC-003 | pass | Existing fix c7c82c514f83538e7624ae12dc286bfd40d78a17 verified, no redundant driver edit. Five actual guider INIT/SHUTDOWN cycles count one MKGP allocation/release per cycle; removing release in disposable mutation fails. Normal and sanitized regression passes. lifecycle-report.md. |
| AC-004 | pass | Three changing-date calls agree with fresh nutation cache, 1e-14 degree tolerance; pre-fix fails. numerical-astro-{baseline,fixed}.log; integrated-regressions.log. |
| AC-005 | pass | Both LibRaw init entry points return LIBRAW_UNSUFFICIENT_MEMORY without dereference/open/cleanup; retained init-success/open-error cleanup verified. Pre-fix UBSan NULL member error. Three cases pass normal/sanitized. numerical-dslr-{baseline,fixed}.log; integrated-regressions.log. |
| AC-006 | pass | Three fixed epochs verify aberration arcsecond-to-degree contract against geometric solar position plus nutation; original fails. No independent full ephemeris accuracy claim. numerical-astro-{baseline,fixed}.log. |
| AC-007 | pass | Sixteen bind failures on an occupied ephemeral loopback UDP port close each new descriptor. Original leaks; fixed candidate unit binary passes. Both platform cleanup branches inspected; Windows runtime not available. io-baseline.log; unit-complete.log. |
| AC-008 | pass | Two new mapping regressions cover overlapping identities and all five explicit CCD simulator devices; targeted 20-test editor set passes. Original corpus mapping failure is gone; full-suite pre-existing defects remain as detailed below. editor-report.md; editor-targeted.log. |
| AC-009 | pass | Original handshake aborts under UBSan signed padding shift. Final high-bit byte vectors (64/65/127 bytes), RFC accept value and key-length limits pass all five cases under ASan/UBSan with halt_on_error. sha-baseline.log; sha-final.log. |

## Executed commands

Working directory unless stated: /private/tmp/indigo-selected-audit-fixes. macOS arm64; Apple clang 21.0.0; Python 3.9.6. Normal native compilation builds x86_64 and arm64; execution/sanitizers run arm64 only. No hardware or external network endpoints.

| Command | Exit/result | Evidence |
| --- | --- | --- |
| make -C indigo_libs -j4 all | 2, environmental FTDI DMG remount failure caused by fresh checkout timestamps | library-build.log; not a pass |
| make -C indigo_libs -j4 -o /private/tmp/indigo-selected-audit-fixes/build/lib/libftd2xx.a all | 0; all candidate core/client/indigocat source rebuilt, only unchanged copied FTDI archive reused | library-build-offline.log |
| make -C indigo_test build/unit/test_uni_io; indigo_test/build/unit/test_uni_io (pre-fix) | Build 0; test 1, only new bind-failure regression fails | io-baseline-build.log; io-baseline.log |
| UBSAN_OPTIONS=halt_on_error=1 make -C indigo_test test-server-websocket-sanitize (pre-fix) | 2, expected signed-shift UBSan abort | sha-baseline.log |
| make -C indigo_test -j4 test-audit-fixes test-audit-fixes-sanitize | 0; eight focused cases normal and eight sanitized | integrated-regressions.log |
| make -C indigo_test -j4 test-unit (initial attempts) | 2, unchanged offline Alpaca archive missing; test run not completed | unit-final.log; unit-final-relinked.log |
| make -C indigo_test -j4 test-unit (after copying unchanged Alpaca archive) | 0; 392 PASS entries, zero FAIL | unit-complete.log |
| UBSAN_OPTIONS=halt_on_error=1 make -C indigo_test test-server-websocket-sanitize (final) | 0; 5 cases pass | sha-final.log |
| ./.factory/scripts/check | 0; whitespace, findings, decision index, project checks | factory-check.log |
| ./.factory/scripts/test | 0; 190 Factory tests pass | factory-test.log |
| ./.factory/scripts/security | 0; configured boundary checks | factory-security.log |
| plutil -lint indigo.xcodeproj/project.pbxproj | 0 | Coordinator and independent integration reviewer output |

Exact developer/reviewer build and targeted editor commands are in numerical-raw-report.md, lifecycle-report.md, editor-report.md and scoped review reports.

## Baseline separation and limitations

Full editor suite: baseline 35 tests had 3 failures and 2 errors with loopback permission; final 37 tests have 2 failures and 2 errors. The requested mapping failure is fixed. Unchanged catalog id and polaralign omissions are tracked by 60d4-005 and 60d4-004. SDK discover catalog omission is newly registered as FND-20261026172255-587a-fix-selected-audit-findings-001, with evidence in follow-up-findings.md. These are visible pre-existing failures, not skipped assertions or a passing full editor suite. No acceptance criterion requires those unrelated catalog repairs.

Build isolation reused copied offline dependencies and generator. After rebuilding, the I/O test was explicitly relinked (io-fixed-build.log); otool confirms /private/tmp/indigo-selected-audit-fixes/build/lib/libindigo.dylib, avoiding the copied original dylib's absolute install name. New library prerequisites prevent stale relinking for the changed existing tests. Normal tests use rebuilt candidate libraries; source-including sanitizer tests instrument the targeted production code, not every dependency. RAW allocator heap-accounting is unavailable in the sanitizer environment and is not leak certification; lifecycle tests count actual scoped frees/releases explicitly.

No Windows/Linux runtime, physical driver/hardware run, full integration suite, full maintained-repository audit, release certification, remote fetch, publication or master integration. Full audit remains due; this task repairs its recorded findings and does not reset cadence. No test was removed, weakened or converted into an unconditional pass.

## Finding reconciliation

All nine requested unique findings have verified fixed disposition, including the stale guider finding whose code fix predates this task. Canonical records retain original report references and add this verification report before lifecycle archival. Existing unrelated findings and the new nonblocking catalog follow-up remain active. No security risk accepted. Committed-range process check and clean committed-candidate verification are delivery gates after the final commit and are reported externally, not claimed here as already executed.
