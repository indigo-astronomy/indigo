# Verification

Worktree: /private/tmp/indigo-20261101204146-d6c8-astrometry-platesolver-exclusion. Base3c137d7840cb965924ba106f083139fba1e6d72d. macOS arm64, hardware-free. Developer results are not independent approval.

## Original baseline and audit

Before production edits, reviewed root/driver/test instructions, driver README, development/driver/test references, source and shared property/lifecycle contracts. Architecture, resources, platform/generator and coverage audit was recorded in plan.md. Source is hand-written API2, with queued index handlers and shared platesolver device/client, child commands/WCS parsing, abort and mutex ownership. All these contracts remain unchanged by the identity fix. Active finding summaries were inspected; existing solver margin, recycled-registration and catalog-pointer findings remain outside this task and were not accepted/closed.

Root commands: make Makefile.inc MAC_ARCH='-arch arm64'; copy unchanged support files per support-provenance.txt; make -C indigo_drivers/agent_astrometry -f ../../Makefile.drv all; make -C indigo_test test-agent-astrometry. Exact compiler/link output in baseline-build.log/baseline-tests.log. First compile lacked copied libraw headers; copied remaining unchanged libraw*.h and rebuilt successfully. Original controlled-tool suite52/54: index abort download and index abort removes partial multi-file index timed out15s waiting for ALERT while state remained BUSY/revision2. Dedicated original-driver repeats returned1/1 and0/1 (baseline-abort-retest.log). No assertion was weakened.

capture-trace.py baseline adds logging only to a temporary copy of the unchanged integration observer and links original locally built production object. Three cases pass: FITS/WCS success, reported solver failures, repeated lifecycle. Property state/item transitions and ordered fake tool arguments retained in baseline-*.trace. Fixture paths and generated image timestamp/address identifiers normalized. Candidate runs3/3 and all three traces compare byte-for-byte equal (candidate-*.trace, trace-comparison.txt). Temporary trace fixtures removed.

## Found defects and fixes

**Requested compatibility defect:** original INIT checked indigo_agent_solver and missed renamed indigo_agent_platesolver. Production fix changes lookup/log strings and increments version0x02000023→0x02000024 (runtime2.0.0.35→2.0.0.36). No ABI, property, cache, helper/tool or solving change. Coordinator reports actual main-rename29-case suite passed against this immutable archive, including both load orders, exclusion and retry; root retains that independent task evidence and will refresh after integration.

**Original local fixture issue:** fakecurl wrote readiness then spawned sleep30, creating a possible race between SIGTERM processgroup termination and descendant creation/stdout inheritance. Direct process-tree capture initially failed under sandbox, so this ordering is an inference, supported by source, intermittent original failures and corrected-fixture repetitions. Coordinator authorized two fixture-only exec sleep replacements. This models one realcurl process; all state/file/abort assertions and15s timeout remain. Separate masked-signal/child-reaper/descendant scenarios are unchanged and pass. check-curl-fixture.py compiles original HEAD production source in a temporary object and repeats all3 index-abort cases10 times:30/30 pass (curl-fixture-repeats.log). No cancellation production fix.

## Candidate checks

Normal build: make -C indigo_drivers/agent_astrometry -f ../../Makefile.drv all, exit0 (candidate-build.log). Source SHA25636d12e9e20f6e2cc7d7d73213e7112f85238295421451f05daecb1b4ee7a5d51; archive SHA2563223203cd988a7f487c5ffffcba5ac1c1e5436debc8300840d3287b45e0c6280. No later production change.

Strict syntax: cc -std=gnu11 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-deprecated-declarations -Wno-sign-compare -Wno-missing-field-initializers -Iindigo_libs -Ibuild/include -fsyntax-only indigo_drivers/agent_astrometry/indigo_agent_astrometry.c; exit0, no output (strict.log). Suppressions cover existing conventions/deprecations; no unrelated source edits.

make -C indigo_test test-agent-astrometry-sanitize:54/54 pass; existing target directly compiles changed production source with ASan/UBSan and runs with ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 (sanitizer.log). No diagnostics. Manifested unchanged support archives are not instrumented; no whole-library/leak claim.

python3 -B tools/run_driver_test.py agent_astrometry: native controlled-tool54/54 pass in both recorder attempts. First attempt could not resolve data.astrometry.net inside sandbox; stopped only its own process group68641 to avoid repeated failed downloads, exit143 (recorded-network-blocked.log). Read-only elevated ps/sample diagnosed install_index wait with idle handler queue, preserved in real-setup-sample.txt. Elevated HEAD for exact public URL http://data.astrometry.net/4100/index-4112.fits returned200, Content-Length5296320. Second authorized elevated recorder retrieved the public index once into shared temporary cache; SHA25659745b09bb8197acf8d3736ef58716017926837a1ae45021af85591bfd30dd33 (public-index-provenance.txt).

## Owner stop and recording exception

Owner explicitly instructed not to repeat image recognition merely for this identity rename. Stopped remaining real cases immediately: identified owned recorder group73731, sent only that group SIGTERM, verified no remaining recorder/test/solver processes for its task/cache. Recorder exit143, not a completed pass. final-recorded.log contains9 completed/passed real cases: solve only, south galactic pole, solve and sync, solve/sync/center, precise GOTO, polar alignment, abort while solving, abort while exposing, abort while guider exposes. Tenth case started and was interrupted;23 cases never started. No further solving or recorder rerun was performed.

Root recorded the explicit owner waiver of full real-image recording in specification/state. Recorder did not reach its README/TEST_SUMMARY write; both last complete historical records remain unchanged. No54/54 or87/87 Testing entry was fabricated. Required remaining quality/security/process/Git work belongs to coordinator; no automatic gate waiver is inferred beyond this explicit owner exception.

## Cleanup and limits

Known task-owned trace/debug fixtures and aborted shared index caches removed after preserving logs/hash. Test processes stopped as verified. Test-clean removes generated suite binaries. Normal driver archive intentionally remains available to coordinator. No source/vendor/generator/protected-driver/shell-compliance edits; no staging/commit. Initial sandbox ps denial was resolved only through approved read-only escalation. Real-image cache and interrupted private HOME cleanup may have been interrupted by SIGTERM; known shared cache was removed, no unrelated temporary directories were swept.

Linux/Windows and physical hardware execution unperformed; no claims of hardware pulse timing or whole application certification. Logs are globally ignored by repository patterns and must be explicitly retained by coordinator when staging evidence.

## Final test summary

Final controlled/simulated native suite:54 run,54 passed. Changed-driver sanitizer suite:54 run,54 passed. Original-driver corrected-fixture repetitions:30 run,30 passed. Candidate trace scenarios:3 run,3 passed; all3 normalized comparisons equal. Partial real-image suite:10 started,9 completed/passed,1 interrupted by owner instruction;23 not started. Physical hardware:0 run,0 passed. Full87-case recorder completion is explicitly waived by owner, not claimed.

Verdict: pass

## Coordinator metadata and final review

Configured .factory/scripts/check and .factory/scripts/security executed from this worktree and passed; exact outputs retained in factory-check.txt and factory-security.txt. Independent scoped final-diff review is retained in quality-review.md/security.md. Required full recognition recording is explicitly waived only by the direct owner instruction in specification.md; no incomplete run is a passing full recording. Final committed-candidate process validation is recorded externally; inherited unfinished audit is not bypassed.

Common unchanged Factory tooling group: ./.factory/scripts/test from main rename worktree passed190/190 in107.958s; exact output factory-test.txt. This validates process tooling only and performs no image recognition.
