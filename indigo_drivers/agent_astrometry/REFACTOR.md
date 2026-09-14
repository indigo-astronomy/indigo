# Astrometry Agent refactoring and test record

## Baseline and current-state audit

The driver is a hand-written platesolver agent. It embeds `platesolver_private_data` as the first member of its private data and delegates common public properties, related-agent discovery, capture, mount sync/slew, polar-alignment state and lifecycle behavior to `indigo_platesolver.c`. Its driver-specific responsibilities are image conversion to FITS, construction and execution of `image2xy` and `solve-field` commands, parsing Astrometry.net output, temporary-file cleanup, process-group abort, and installation/removal of 41xx Tycho-2 and 42xx 2MASS index families. It exposes the two driver-specific AnyOfMany index-management properties in addition to the shared platesolver properties.

The implementation is compiled for macOS, Linux and Windows. POSIX uses `fork`, a process group, `/bin/sh`, pipes and `curl`; Windows uses `CreateProcess`, a kill-on-close job object and WinINet for index downloads. It is not generator-backed. Build integration exists in the generic driver Makefile, Xcode and Visual Studio projects. Repository documentation consists of `README.md`, the shared platesolver API/property declarations and Astrometry.net command-line output consumed by the parser. The README is user-facing and will not be changed without explicit approval.

Concurrency and ownership risks are concentrated in the asynchronous shared platesolver task, `config_mutex`, child-process state, abort/shutdown, additional agent instances, temporary image ownership and index handlers. Image inputs may be FITS, INDIGO RAW1/RAW2/RAW3/RAW6, JPEG or DSLR RAW. Solver success depends on parsed output setting `failed = false`; command execution and image/index I/O have multiple terminal-error paths.

No driver-specific automated test existed at baseline. The Sequencer corpus only uses a deterministic astrometry peer and does not load this driver. No fake Astrometry.net command suite, parser fixtures, index-management regression, image-format matrix, abort/lifecycle test or sanitizer run existed. The host has `/usr/bin/curl` but no `solve-field` or `image2xy`, so a normal driver INIT cannot run without a controlled fake executable environment.

Baseline on 2026-09-14, Darwin arm64 host: `make -C indigo_drivers/agent_astrometry -f ../../Makefile.drv all` built the universal macOS arm64/x86_64 archive and dylib successfully. There were 0 existing driver-specific tests to run. No physical hardware will be tested: the agent has no direct hardware interface, and camera/mount behavior will be represented by deterministic public-bus peers or repository simulators. No hardware validation is claimed.

## Atomic plan

1. Inventory every shared and driver-specific property, operation, parser result, image input, solver option, index transition, failure path and lifecycle/concurrency path; map them to concrete hardware-free cases — initial inventory complete. The first matrix covers schema/settings, FITS/RAW/JPEG, WCS parser units and errors, direct and related-agent capture, solve/sync/center/precise GOTO, abort/recovery, index install/remove/download failures and shutdown during solve. Polar-alignment transitions, additional instances and several command-option combinations remain to be added when work resumes.
2. Add a driver-specific `test_agent_astrometry.c` integration corpus plus controlled fake `image2xy`, `solve-field` and `curl` executables; register all persistent files in the test Makefile and Xcode — in progress. The source and Makefile target exist and build; Xcode registration is pending. The initial release baseline executed all 20 cases and passed 14/20. This is an intentionally incomplete red baseline before production fixes.
3. Cover metadata, enumeration, configuration/reset, related-agent selection, direct uploads and peer capture, image conversions, solver hints/output units/errors, solve/sync/center/precise-goto, abort/recovery, index add/remove/failure, multiple instances and shutdown — pending.
4. Reproduce and fix every in-scope defect, increment the driver version for behavior fixes, and add a focused regression for each defect — pending.
5. Run the release corpus, strict warning build, ASan/UBSan coverage, universal production build and available project validation; document unavailable platforms and residual limitations — pending.
6. Update review records, `MIGRATION_STATUS.md`, project registration and this scenario mapping; remove generated test/build artifacts and reconcile final totals — pending.

## Found defects

- Reproduced: truncated RAW payloads and RAW headers with overflowing dimensions are accepted far enough to read beyond the supplied BLOB and can finish with WCS OK. Impact: malformed client/camera data can cause out-of-bounds reads, excessive allocation, integer overflow or a crash. Root cause: `astrometry_solve()` identifies signatures and consumes dimensions/pixels before validating the minimum header size, positive bounded dimensions, multiplication overflow and complete payload length. Production fix is pending. Regressions: `short truncated and broken images` and `invalid RAW dimensions`.
- Reproduced: a nonzero `image2xy` exit with no recognized output is treated as success, so `solve-field` still runs and the solve can finish OK. Impact: executable and I/O failures can be reported as successful astrometry. Root cause: the POSIX `execute_command()` does not reap the child or inspect its exit status; the Windows path waits but also does not inspect the process exit code. Production fix is pending. Regression: `child process exit status`.
- Investigation pending: `abort and recovery` reaches ALERT for the aborted solve but the immediate follow-up solve remains ALERT in the initial harness. Determine whether this is an agent cleanup race or whether the test must wait for the solving worker/config mutex to finish before retrying.
- Test expectation corrections pending and not classified as driver defects: `INDIGO_DRIVER_INFO` returns the driver identifier in `info.name`, while the initial schema assertion expected the device display name; the `solve-field: not found` parser path logs an error rather than publishing that exact text as a client message.

## Scenario mapping

- Schema/settings: metadata, the two index families, key shared property types/counts, epoch normalization, exposure/PA setting synchronization and factory reset.
- Images: FITS pass-through, RAW1/RAW2/RAW3/RAW6 conversion, JPEG conversion, empty BLOB, short/truncated/broken inputs and dimension overflow.
- Solver execution/parser: successful center/size/angle/index/parity, degree/arcminute/arcsecond units, solver messages, no solution, no index, CPU limit, command-not-found output and nonzero child exit.
- Orchestration: direct upload target copy, related Imager capture, missing image source, Mount sync/center, precise GOTO, abort/recovery and shutdown during a running solver.
- Index management: successful install/remove, failed transfer and invalid downloaded signature with partial-file cleanup.
- Still required: complete property/item schema, all hint argument combinations and generated config contents, image write failures, solver concurrency, peer failure/abort paths, polar alignment including failure/recalculation, additional instances, repeated lifecycle and sanitizer-specific cases.

## Final test summary

Simulated/fake hardware-free tests run/passed: 20 / 14 initial red baseline. Six failures consist of two confirmed production-defect groups, one abort/recovery investigation and two incorrect test expectations; no completion claim is made.

Physical hardware tests run/passed: 0 / 0.
