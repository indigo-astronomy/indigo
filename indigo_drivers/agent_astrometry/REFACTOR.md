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
