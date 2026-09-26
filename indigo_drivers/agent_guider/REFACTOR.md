# Guider Agent refactoring and validation record

Status: hardware-free validation complete. Production defects `DRV-124` through `DRV-133` and `DRV-182` are fixed and focused-tested. Hardware validation was not performed.

## Current-state audit

- The Guider Agent is a hand-written agent/filter driver tested through the public INDIGO bus with the production agent, filter and CCD simulator. The test boundary keeps real agent timers, image analysis, correction algorithms and configuration paths, with a test-local configuration directory.
- `integration/test_agent_guider.c` contains fork-isolated cases. Each child starts INDIGO independently, owns temporary files/logs and performs cleanup; the parent removes child artifacts even after assertion failures, signals or watchdog timeouts.
- Normal preview images come from the CCD simulator. A deterministic camera boundary supplies Gaussian-star or full-frame centroid fixtures, and a guider spy records pulse direction and duration, translating software pulse commands into image displacement at 10 px/s. This is software boundary evidence, not physical relay timing or real mount motion.
- Covered behavior includes metadata and public properties, missing peer preconditions, one-shot and continuous preview, RAW/FITS restoration, retries, camera disconnection/reselection, star selection, detection/correction modes, pulse direction and thresholds, DEC modes, calibration, loss-of-star policy, dithering, PPEC, Multi Kernel GP (own settings, statistics, reset and learned model, kept across switches between PPEC and MKGP), logging, configuration reload, additional instances, related-mount coordination and active shutdown.
- Remaining gaps include physical mount/camera timing, long-period PPEC prediction/convergence, full-duration timeout paths, forced allocation/attach failure, real network/BLOB download transport, serial/USB behavior of underlying drivers and Linux/Windows execution.

## Recorded test evidence

- Initial 64-case integration coverage: `make -C indigo_test test-agent-guider` ran on macOS arm64 against production source `ba9259e22`; 53 of 64 cases passed and eleven regressions failed. The target returned nonzero as expected, and known failures were not masked.
- Initial sanitizer reproduction used a separate `TEST_BUILD=build/guider-asan` build with `GUIDER_TEST_FLAGS='-fsanitize=address -fno-omit-frame-pointer -O1'`. The `truncated raw` and `shutdown active` sanitizer regressions reproduced; `live dec mode guards`, `centroid resist switch guiding` and `spiral sequence reset` passed under instrumentation.
- `DRV-124` RAW validation: all four RAW-filtered cases passed normally and with AddressSanitizer. Cases cover truncated headers, zero/oversized dimensions, payload-short MONO8/MONO16/RGB24/RGB48 and recovery.
- `DRV-125` active teardown: shutdown and instance selections passed, covering shutdown during subframe/exposure, active primary and secondary instance teardown, active additional-instance removal, configuration reload and simultaneous agents. These passed both normally and with AddressSanitizer for the selected set.
- `DRV-126` single-preview completion: five focused cases passed, covering invalid RAW, failure, retry, abort/restart and ordinary success with settings restoration.
- `DRV-127` continuous-preview completion: six focused cases passed, covering exposure failure, invalid RAW, abort status, camera disconnect, shutdown exposure and simultaneous agents.
- `DRV-128` adaptive calibration step: six focused cases passed, including adaptive minimum/maximum/step, calibration no motion, calibration abort and calibration-and-guiding.
- `DRV-129` calibration speed: six focused cases passed, including speed accuracy, single-pulse speed, directional speed, adaptive step, RA-only calibration and calibration-and-guiding.
- `DRV-131` delay abort: four focused checks passed: `guiding delay` 2/2, `correction response` 1/1 and `shutdown active` 1/1.
- `DRV-132` pulse failure propagation: eleven pulse-filtered cases and three star-loss policy cases passed. Coverage includes transient RA/DEC recovery, DEC error isolation, completion timeout, timeout abort, calibration RA/DEC failure and calibration-and-guiding failure.
- `DRV-133` zero-drift processing: 17 focused cases passed, including zero-drift statistics, zero-drift PPEC learning, correction response, PI integral history, PPEC reset, selection cases, dithering cases, transient-pulse recovery and pulse thresholds.
- `DRV-145` empty-start extension: the Guider `empty start`, single preview, preview abort/restart, BUSY guards and reset defaults checks passed as part of an 11/11 Mount/Imager/Guider/shared-platesolver validation on macOS arm64, with macOS arm64/x86_64 builds.
- The 2026-09-14 Sequencer sanitizer run reproduced `DRV-182`: frame metadata arriving before binning caused division by zero while caching selection dimensions. The positive-binning guard and simulator-guiding regression pass under UBSan.
- 2026-09-26 Multi Kernel GP separation: MKGP got its own settings, statistics, `AGENT_GUIDER_RESET_MKGP` and learned model, separate from PPEC. Only the model of the selected mode takes part in a guiding session, so the other one keeps its stop time for the retain decision. The new `mkgp separate model, reset and mode switch` case covers MKGP-only settings/statistics, PPEC runs and resets leaving the MKGP model alone, MKGP model retention across a PPEC run, and deferred and idle MKGP resets. A mutation that made MKGP use the PPEC model failed the case as expected. The complete `make -C indigo_test test-agent-guider` run passed 89 / 89 cases on macOS arm64. `test-agent-scripting-sequencer` passed 25 / 25 in four of five runs; one run failed in an unidentified case that was not captured. The driver version is `0x03000031`.
- The subsequent complete 88-case run passed 87 unaffected cases and reproduced deferred `DRV-130`. The proposed sine/cosine unit-vector projection was declined for now, so `dither RA projection preserves magnitude` remains the one known failure. The Sequencer calibration/guiding scenario passes because it does not assert RA-only projection magnitude. The driver version is `0x0300002F` for the independently fixed DRV-182 discovery issue.

## Acceptance boundary

- DRV-130 is an accepted known failure awaiting manual approval. Keep `dither RA projection preserves magnitude` as a regression that records the issue, but do not change the RA-only dithering implementation merely because this case fails during future Guider Agent testing or refactoring.
- The test suite validates the Guider Agent's public bus behavior, correction logic, process state, fake camera boundary and software pulse handling. It does not validate physical ST4/electrical relay accuracy, real mount response, physical camera timing, or manufacturer-specific serial/USB guide-port behavior.
- Sanitizer results instrument the test, agent, filter and selected framework/configuration boundary used by the test. They do not claim whole-program instrumentation of prebuilt simulator or library archives.
- Hardware tests were not run. No hardware validation is claimed.

## Final test summary

- Simulated/fake hardware-free tests: 89 / 89 passed in the 2026-09-26 run. Before that, 87 / 88 passed. The sole failure, `dither RA projection preserves magnitude`, records the open and intentionally deferred DRV-130 finding. DRV-182's focused UBSan and Sequencer simulator-guiding regressions passed.
- Hardware tests: 0 executed, 0 passed.

## Linux agent test run (2026-09-24)

- Environment: Linux x86_64 (Ubuntu, GNU ld 2.42), simulator only, strict bus locking always on (the framework's `indigo_use_strict_locking` switch was removed in the same session).
- `dither RA projection preserves magnitude` passes: DRV-130 is closed as fixed in `indigo_drivers/REVIEW.md`, so the accepted-failure note above is historical.
- `selection subframe restore` failed deterministically because the CCD simulator never published the sensor size set through `SIMULATION_SETUP` (see `ccd_simulator/REFACTOR.md`, 2026-09-24). Fixed in the simulator; no Guider Agent change.
- `dither strategies` hung in about one of ten runs (SIGALRM). A thread dump showed a test-harness deadlock: the fake camera called `indigo_cancel_timer_sync()` from `change_property` with the bus locked while its `synthetic_frame()` timer waited for the bus lock in `indigo_update_property()`. The fake camera now aborts on its device queue, as `DRIVER_DEVELOPMENT_BASICS.md` requires; the case then passed 30 of 30 repetitions.
- Cooperation with the production Imager and Mount agents (dithering handshake, aborts while dithering, Mount Agent `ABORT_RELATED_PROCESS` on slew and park) is covered by `integration/test_agent_imager_guider_mount.c`, see `agent_imager/REFACTOR.md`.
- Result: 88 / 88 simulated cases passed; hardware tests 0 / 0.
