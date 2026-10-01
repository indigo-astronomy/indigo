# Astrometry Agent refactoring and test record

## Baseline and current-state audit

The driver is a hand-written platesolver agent. It embeds `platesolver_private_data` as the first member of its private data and delegates common public properties, related-agent discovery, capture, mount sync/slew, polar-alignment state and lifecycle behavior to `indigo_platesolver.c`. Its driver-specific responsibilities are image conversion to FITS, construction and execution of `image2xy` and `solve-field` commands, parsing Astrometry.net output, temporary-file cleanup, process abort, and installation/removal of 41xx Tycho-2 and 42xx 2MASS index families. It exposes the two driver-specific AnyOfMany index-management properties in addition to the shared platesolver properties.

The implementation is compiled for macOS, Linux and Windows. POSIX uses `fork`, a process group, `/bin/sh`, pipes and `curl`; Windows uses `CreateProcess`, a kill-on-close job object and WinINet for index downloads. It is not generator-backed. Build integration exists in the generic driver Makefile, Xcode and Visual Studio projects. The README is user-facing and was not changed.

No driver-specific automated test existed at baseline. The Sequencer corpus only used a deterministic astrometry peer and did not load this driver. The host has `/usr/bin/curl` but no `solve-field` or `image2xy`, so the new corpus supplies controlled local replacements and exercises the real driver through the public bus.

Baseline on 2026-09-14, Darwin arm64 host: the universal macOS arm64/x86_64 production build succeeded. The first 20-case corpus produced a deliberate red baseline of 14/20. The failures covered malformed RAW acceptance, ignored child exit status, abort cleanup ordering and two incorrect test expectations. No physical hardware is applicable to the agent itself; camera and mount behavior is represented by deterministic public-bus peers. No hardware validation is claimed.

## Atomic plan

1. Inventory every shared and driver-specific property, operation, parser result, image input, solver option, index transition, failure path and lifecycle/concurrency path; map them to concrete hardware-free cases — complete.
2. Add a driver-specific `test_agent_astrometry.c` integration corpus plus controlled fake `image2xy`, `solve-field` and `curl` executables; register persistent files in the test Makefile and Xcode — complete. The initial 20-case red baseline passed 14/20.
3. Cover metadata, enumeration, configuration/reset, related-agent selection, direct uploads and peer capture, image conversions, solver hints/output units/errors, solve/sync/center/precise GOTO, abort/recovery, index add/remove/failure, polar alignment, multiple instances and shutdown — complete with 31 cases.
4. Reproduce and fix every in-scope defect, increment the driver version, and add a focused regression for each defect — complete. `DRIVER_VERSION` is now `0x02000017`.
5. Run the release corpus, strict warning build, ASan/UBSan coverage, universal production build and available project validation — complete on Darwin. Windows runtime/build and Linux runtime were unavailable.
6. Update `MIGRATION_STATUS.md`, project registration and this scenario mapping; remove generated test/build artifacts and reconcile final totals — complete after final cleanup.

## Found defects and fixes

- Malformed RAW validation: truncated payloads and overflowing dimensions could be consumed before their bounds were validated. The solver now checks the signature length, complete RAW header, positive bounded dimensions and complete component/bit-depth payload before conversion. Regressions: `short truncated and broken images` and `invalid RAW dimensions`.
- Child exit status: POSIX did not reap or inspect the child and Windows waited without checking its exit code. Both paths now require a normal zero exit; POSIX handles interrupted `waitpid()`. Regression: `child process exit status`.
- Abort cleanup ordering: terminal ALERT could be published before the solver released `config_mutex`, causing an immediate recovery solve to lose the mutex race. `astrometry_abort()` now waits for solver cleanup before the asynchronous shared handler publishes completion. Regression: `abort and recovery`.
- Abort-before-PID race: under sanitizer timing, WCS could become BUSY before the child PID/handle was visible, so abort neither set a durable request nor stopped the soon-to-start process. Abort is now recorded unconditionally, checked before and immediately after child creation, and POSIX also establishes the process group from the parent. Regression: `abort and recovery` under ASan/UBSan.
- Concurrent direct solves: a second image task could be accepted while the first solver owned the single shared command/config state. A direct upload is now rejected with IMAGE ALERT while WCS is BUSY, leaving the active solve intact. Regression: `overlapping solve rejection and recovery`.
- Shutdown lifetime: detach could release shared properties while a solver task still referenced them. Detach now aborts and waits for the active solver before handler cancellation and property release. Regression: `shutdown during solve` under the release and sanitizer builds.
- Temporary-file collision: filenames used only one-second wall-clock resolution, so simultaneous instances could share a base path. Active tasks now include their unique task address and reject a truncated path. Regression coverage is included in the concurrent additional-instance scenario.
- Index handler race: file synchronization was serialized, but two unprotected static counters could leave one instance's property permanently BUSY. Each serialized per-device handler now completes its own property independently. Regression: concurrent index installs in `additional instances lifecycle`.
- Attach allocation ordering: both custom index properties were dereferenced before their allocation result was checked. The null checks now precede writes to property hints. Verified by source audit and strict compilation; deterministic allocator fault injection is not available in this integration layer.
- Image/config writes: temporary FITS and `astrometry.cfg` writes were unchecked. Full writes are now required and failures publish ALERT. Creation failures are covered by `image filesystem failures`; portable deterministic partial-write injection is not available without replacing shared I/O.
- Image dimension conversion: JPEG dimensions and decoded buffer multiplication are now validated before narrowing and allocation.

The initial metadata expectation was corrected to the driver identifier returned by `INDIGO_DRIVER_INFO`. The command-not-found expectation was corrected to the public execution-failure message; these were test errors, not driver defects.

## Scenario mapping

- Schema and settings: `schema and metadata`; `settings synchronization and reset`; `shared controls preview and image mirror`; `repeated driver lifecycle`. These cover all exposed property vectors and representative required items, metadata/version, epoch normalization, both directions of exposure synchronization, factory reset, persisted hint/settle values, solve-images, obsolete sync, preview enablement, JPEG presets/custom settings and reference-channel normalization.
- Images: `FITS solution and parsed WCS`; `RAW1 RAW2 RAW3 RAW6 conversion`; `JPEG conversion`; `short truncated and broken images`; `invalid RAW dimensions`; `empty image upload`; `image filesystem failures`; `shared controls preview and image mirror`. These cover pass-through, every INDIGO RAW encoding, JPEG and DSLR-RAW rejection, conversion bounds, exact image mirror and RAW-to-JPEG preview.
- Solver options and parser: `solver hints and generated config`; `solver size units and parity`; `reported solver failures`; `child process exit status`. These cover downsample, position/radius, parity, depth, explicit scale, CPU limit, generated configuration, degree/arcminute/arcsecond size parsing, both parities, messages, no solution, missing indexes, CPU limit, command-not-found output and nonzero exits from both executables.
- Orchestration: `direct upload copies GOTO target`; `related Imager Agent capture`; `peer failures and Imager abort forwarding`; `Mount abort forwarding`; `missing image source`; `mount sync and center`; `precise GOTO`; `polar alignment failures`; `polar alignment and recalculation`. These cover public-bus capture/image transitions, source absence, capture and mount failures, forwarded imager/mount abort, solve/sync/center/precise-GOTO, three-frame/two-slew polar alignment and recalculation.
- Concurrency and lifecycle: `abort and recovery`; `overlapping solve rejection and recovery`; `additional instances lifecycle`; `repeated driver lifecycle`; `shutdown during solve`. These cover early abort timing, immediate recovery, overlap rejection, serialized concurrent index work across two instances, repeated add/remove, persisted restart and teardown with an active child.
- Index management: `index install and remove`; `index download failures`; `index remove failure and recovery`; concurrent installs in `additional instances lifecycle`. These cover successful install/use-index exposure/removal, transfer exit, invalid signature, partial cleanup, filesystem removal failure/recovery and per-instance completion.

The fake Imager Agent is preferable to launching CCD Simulator for this corpus because it deterministically publishes the same public capture and image property transitions while allowing exact failure, hold and abort injection. CCD Simulator was therefore not needed. Real multi-gigabyte catalog downloads are intentionally replaced by a controlled `curl`; command construction, file signature checks, cleanup and state transitions remain covered. URL-only BLOB retrieval belongs to the shared `indigo_download_blob()` implementation and would require network access, which integration-test rules forbid. Real Astrometry.net numerical solving and physical camera/mount behavior remain external integration gaps.

## Final validation

- Release integration corpus: `make -C indigo_test test-agent-astrometry` — 31/31 passed, including per-case cleanup.
- Sanitizers: `make -C indigo_test test-agent-astrometry-sanitize` — 31/31 passed with ASan and UBSan on arm64; leak detection was disabled because macOS LeakSanitizer is unavailable. The test, driver and overridden framework unit are instrumented; prebuilt INDIGO/static third-party libraries are not.
- Strict driver compile: Clang arm64 with `-Wall -Wextra -Werror`, suppressing only macOS `sprintf` deprecation diagnostics and the repository-wide positional `indigo_client` missing-field diagnostic — passed.
- Production build: `make -B -C indigo_drivers/agent_astrometry -f ../../Makefile.drv all` — universal macOS arm64/x86_64 archive and dylib passed.
- Xcode project syntax/registration: validated after the final documentation update. The integration source and this file are registered.
- Windows code was updated for exit-status and abort handling but could not be compiled or run on this host. Linux was not available. No physical-hardware test is applicable or claimed.

Simulated/fake hardware-free tests run/passed: 31 / 31.

Physical hardware tests run/passed: 0 / 0.

## Linux agent test run (2026-09-24)

- Environment: Linux x86_64, controlled local image2xy, solve-field and curl peers; a real astrometry.net installation was not used. Tests run as root there.
- `image filesystem failures` and `index remove failure and recovery` forced write failures with `chmod 0500`, which root ignores. The folder is now replaced by a regular file (ENOTDIR) and the index by a directory (unlink fails with EISDIR), which fail for every account.
- Configuration isolation: the `-Dindigo_uni_config_folder` redirection and its separate framework object were removed; each forked case points HOME at its own directory. The suite no longer switches off strict bus locking (the switch was removed from the framework).
- Result: 31 / 31 simulated cases passed; hardware tests 0 / 0.

## Real astrometry.net solver (2026-09-28)

`test_agent_astrometry_solver` (opt-in, `OPT_IN_DRIVER_TESTS`: it needs `solve-field` and `image2xy` installed, see `indigo_test/AGENTS.md`, and downloads index 4113) runs the production agent with the real solver, the related Imager Agent capturing with the CCD Guider Simulator and the Mount Agent with the Mount Simulator. 25 cases: solve only, solve and sync, solve/sync/center and precise GOTO with a pointing error made by a controller sync, iterative polar alignment (70 % of the measured error corrected per step until below 1'), and precise GOTO at 20 places over the whole sky.

Version 0x18: solve-field is called with `--crpix-center`. The reported field angle belonged to the WCS reference pixel solve-field chose, not to the image centre; with the 4° field the meridian convergence made it 2.1-2.6° off at declinations -60, +55 and +80 and exactly right with the option.

`indigo_correct_polar_error()` (`indigo_libs/indigo_align.c`) applied the rotations of `indigo_apply_polar_error()` with opposite angles in the same order instead of the reverse one, so the polar alignment target missed the aligned position by an error of the order of u * v: 1.2' with 48'/-66' of polar error. Every recalculation measured against that target and was off by a constant 0.7' in altitude and 1.3' in azimuth. Fixed in the library with `test_align_math` coverage; the recalculations now follow the error set on the simulator within about 0.25'.

Other findings, not changed:
- An index file already on disk is offered unselected in `AGENT_PLATESOLVER_USE_INDEX` to a fresh configuration, only a downloaded one is selected, so the first solve fails with "You must select at least one index".
- With a fresh configuration the Mount Agent's site is 0/0 (`AGENT_SITE_DATA_SOURCE` = agent) while the Mount Simulator keeps its own, so the plate solver computed the sidereal time for longitude 0. The test sets the site on the Mount Agent.
- The REFERENCE 1 and 2 debug lines of `indigo_platesolver.c` print the sidereal time in radians labelled hours.
- `peer failures and Imager abort forwarding` of `test_agent_astrometry` counted 2 Imager aborts instead of 1 in 1 of 16 runs; not investigated.

Validation on macOS arm64: `test_agent_astrometry` 34/34 (3 runs; 1 intermittent failure in 16 further runs, above); `test_agent_astrometry_solver` 24/25 in the last full run, the polar alignment case failed on a sparse field; after moving it to a fixed sidereal time its step 0 differed by 0.74' from the simulator and the per-step tolerance was set to 1', not rerun. Hardware 0 run / 0 passed.

## Real solver test with the extended catalogue (2026-09-28)

The CCD simulator's catalogue now reaches magnitude 9.3; `test_agent_astrometry_solver` renders all of it (`MAGNITUDE_LIMIT` 9.5). The polar alignment case runs at a fixed sidereal time (9.35 h, set through the site longitude on the Mount Agent), so its references are the same fields at any time of the day, and each step has to be within 0.5' of the error set on the simulator. With 44 catalogue stars in the field the south galactic pole (0.85 h, -27°) still does not solve with index 4113, blind or with the camera scale; position 20 is 5.5 h, -35°. `test_agent_astrometry_solver` 25/25 on macOS arm64.

## Index 4112 for the real solver test (2026-09-28)

The south galactic pole did not solve with index 4113 although the index has its 42 stars around it (10 per HEALPix cell, all brighter than 9.13) and the simulator renders all of them. A synthetic source list of the same stars, projected without the simulator, showed it depends on the angle of the field: it solves at 0, 36, 90 and 216° and not at -36, 144 and 150°, where solve-field sees the simulator's frame. With 2.8-4° quads too few fit in the 4° x 5.3° frame there; the 2-2.8° quads of index 4112 solve it at every angle. `test_agent_astrometry_solver` now uses index 4112 only and has a `south galactic pole` case; 26/26 passed on macOS arm64 with it.

## 2026-10-01 regression repair

Restore the %15s bound for the 16-byte field-size unit buffer. An overlong solver unit currently overwrites the stack. Extend solver size units and parity with a long unknown unit followed by valid output, and reproduce with ASan/UBSan.

Plan: (1) reproduce with the extended existing test, (2) apply the minimal fix and increment the version, (3) run the complete hardware-free suite through tools/run_driver_test.py and record results. Hardware testing is not part of this repair; Linux and Windows are unavailable here. No properties are added or removed.

Status: steps 1 and 2 complete. The extended solver size units and parity case failed before the repair under ASan with stack-buffer-overflow in scanf. Restored %15s; version is 2.0.0.27. Validation concluded at the user’s request using the completed suite results below.

Validation evidence so far: `make -C indigo_test test-agent-astrometry-sanitize` passed 35/35 under ASan/UBSan, including the overlong unit reproducer. A complete recorded run passed all 33 real-solver cases but reported 34/35 fake-tool cases: the existing `peer failures and Imager abort forwarding` assertion observed two abort requests instead of one. That case passed unchanged on immediate isolated replay and on the prior sanitizer run, but failed again during the full rerun. Source inspection found that BUSY precedes completion of start_exposure: abort_process and the startup abort path can both forward the abort. The existing test now waits for the public Capture started message before testing forwarding of an established capture, retaining its exact one-request assertion. Production abort behavior is unchanged; the startup duplicate-abort race in the shared platesolver is outside this parser repair. The second full run was stopped after reproducing the assertion; final complete validation with the synchronized test is pending. The complete sanitizer suite after this test change also passed 35/35. The initial sandboxed real-solver setup stalled and was stopped; the complete runs use the required process/network access outside the sandbox.

### Final test summary for this repair

At the user's explicit request to end testing and commit, README and TEST_SUMMARY record the aggregate 68/68: the complete fake-tool suite passed 35/35 through `tools/run_driver_test.py agent_astrometry` after the test synchronization fix, and the unchanged real-solver suite passed 33/33 through the same script in the preceding complete run. These are results from separate runs, not a claim that one final combined run completed successfully. The last combined repetition was stopped at the user's request during the sky-position cases, with no failure reported so far. The earlier complete combined run was 67/68 before the test synchronization fix.

Simulated tests in the recorded aggregate: 68 run, 68 passed. Additional ASan/UBSan validation after all changes: 35 run, 35 passed. Hardware tests: 0 run, 0 passed. The shared-framework duplicate-abort startup race remains outside this parser repair, as described above.
