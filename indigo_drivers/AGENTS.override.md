# Driver Refactoring Rules

## Scope and precedence

- These instructions apply to every driver subtree under `indigo_drivers/`, except `ccd_apogee`, `ccd_sbig`, `mount_asi`, `mount_mxhd` and `system_ascol`, which must never be modified (see the root `AGENTS.md` scope rule).
- Follow the repository-root `AGENTS.md` as well. This file specializes its rules for driver refactoring; it does not relax repository-wide requirements, generated-driver rules, testing standards, formatting, or repository hygiene.
- Read the relevant repository and driver documentation named by the root instructions before changing behavior. For automated tests, also follow `indigo_test/AGENTS.md` and `indigo_test/DRIVER_TESTING_RULES.md`.

## Mandatory refactoring record

- Every driver refactoring MUST begin by creating or updating `REFACTOR.md` directly in that driver's directory, before any production-code change. Do not start implementation until the baseline, audit, hardware-test decision, and initial atomic plan below are recorded.
- `REFACTOR.md` MUST audit the current state, including:
  - driver architecture and implementation, public properties, observable behavior, supported capabilities, protocol or vendor SDK, and available manufacturer or repository documentation;
  - known and newly discovered defects, risks, blocking and asynchronous behavior, synchronization and concurrency, error handling, and resource/lifecycle ownership;
  - supported operating systems and architectures, build and packaging integration, and any relevant generator inputs and generated outputs;
  - existing simulators, fake SDKs, automated tests, hardware tests, and concrete coverage gaps.
- Before implementation, establish the baseline by building the existing driver and running its existing relevant tests when available. Record the exact commands, environment or architecture, result, and any pre-existing failure or unavailable prerequisite in `REFACTOR.md`.
- Before implementation, explicitly decide whether hardware testing will be performed. If yes, record the exact device/model and the planned hardware scenarios. If no, record that decision explicitly and do not claim or imply hardware validation.
- `REFACTOR.md` MUST contain an explicit, logically ordered plan of small, atomic, independently verifiable migration/refactoring steps. Include documentation, simulator or fake-SDK work, test coverage, project registration, generation, and final verification steps where applicable.
- Immediately after performing EACH plan step, update that step in `REFACTOR.md` with its state and actual result, relevant build/test evidence, discoveries, and deviations from the plan. Never mark a step complete before its stated verification has actually passed; record blocked or failed steps honestly.
- `REFACTOR.md` MUST contain a dedicated found-defects section. For every defect discovered during the refactoring, record the observable impact, root cause, concrete production-code fix, and the regression test that proves the fix. Distinguish reproduced failures from defects found only by source audit, and keep this section current as further defects are discovered.
- At the end of every `REFACTOR.md`, include a final test summary that states separately the total number of simulated tests run and passed and the total number of hardware tests run and passed. Use explicit zero counts when a category was not run, and keep the totals consistent with the recorded test evidence.

## Required test infrastructure and coverage

- Every driver migration MUST use a characterization-first workflow. Before changing production code, generator input, generated output or lifecycle behavior, complete the applicable simulator or fake-SDK implementation and the driver-class acceptance coverage against the original driver.
- The pre-migration suite MUST pass against the original driver for every behavior intended to be preserved. A known defect that cannot pass MUST have a dedicated reproducer recorded as an expected baseline failure in `REFACTOR.md`; do not weaken the test or silently encode the defect as correct behavior. Turn that same reproducer into a passing regression test when the defect is fixed.
- Before changing production code, capture and retain a reference trace from the original driver under the simulator or fake SDK. The trace MUST include public property state/value transitions and ordered protocol or SDK interactions, including arguments, buffer sizes, start/stop/drain ordering, error paths and lifecycle/resource events applicable to the driver. Normalize nondeterministic timestamps, addresses and identifiers so the trace can be compared after migration.
- Treat the exact order and manner of protocol, transport and vendor-SDK calls in the original driver as a significant compatibility hint and presume they exist for a valid hardware reason, even when a call appears redundant or a different sequence looks cleaner. "Manner" includes call conditions, repetition, timing and synchronization, argument derivation, buffer sizing, start/stop/drain placement, which readbacks are trusted or merely diagnostic, and how return values affect control flow. Preserve both order and manner by default. Do not remove, combine, defer, reorder or reinterpret such calls unless a dedicated characterization test and the original-driver reference trace demonstrate equivalent observable behavior for success, failure and repeated-operation paths. Record the evidence and justification in `REFACTOR.md`; absent that evidence, the original behavior is mandatory.
- Treat the original-driver tests and normalized reference trace as the migration compatibility contract. Run the same suite and trace comparison after each behavior-changing migration step. Document and justify every intentional difference in `REFACTOR.md`; an unexplained trace difference blocks completion even when final property states match.
- When compatible hardware is available, run and record the planned non-destructive hardware baseline on the original driver before production changes, then repeat the same scenarios after migration. A user-supplied log is supplementary evidence, not a substitute for the simulator/fake-SDK contract. If hardware, an SDK, or a supported platform is unavailable, record the exact limitation, but this does not waive the mandatory hardware-free characterization suite.
- Do not begin by extracting, rewriting or replacing the driver and then add tests around the migrated result. Reverse extraction for a generated migration may start only after the original implementation is covered and its reference trace has been recorded.
- Name every driver-specific automated-test source and executable target with the exact driver name, for example `test_aux_dsusb_sdk.c` and `test_aux_dsusb_sdk`. Do not place driver coverage in generic shared test sources such as `test_usb_outputs.c`; shared helpers may contain reusable harness code, but each driver's registered cases must live in its conventionally named test file.
- For every serial or hidapi driver, create or maintain a deterministic protocol simulator and complete applicable simulator-backed coverage. Cover the happy path, command and reply handling, malformed replies and transport/protocol failures, lifecycle and reconnect, and abort, overlapping operations, timing, and race behavior when relevant.
- For every SDK-based driver, create or maintain a fake SDK and complete applicable fake-SDK-backed coverage. Cover successful SDK behavior, SDK errors, lifecycle, hot-plug and multiple devices, and concurrency, races, and timing when relevant.
- For every driver that exposes a guider interface, whether standalone or as an additional logical device of a camera, mount, AO, or other driver, run a guiding-pulse duration accuracy test and record the result in that driver's `REFACTOR.md`. Follow the Guider timing measurement rules in `indigo_test/DRIVER_TESTING_RULES.md`; state the requested durations, measured endpoints and workload, sample count, signed and absolute error statistics, and whether the measurement covers simulator/fake-SDK software timing or physical relay output. Never present simulator, SDK-entry, or public-property completion timing as hardware pulse accuracy.
- If a simulator or fake SDK already exists, audit it against the documented protocol or SDK and extend it to close identified gaps. Preserving only an existing smoke test is insufficient.
- Treat physical hardware and the real vendor SDK as sources of required simulator behaviour, not only as test targets. Every behaviour a hardware or real-SDK run reveals, in particular each defect it finds, MUST be reproduced in that driver's simulator or fake SDK together with a regression test, and the hardware observation it came from MUST be named in `REFACTOR.md`. Follow the modelling rules in `indigo_test/AGENTS.md`. Record the reason explicitly when a behaviour cannot be reproduced hardware-free, and do not count that scenario as covered.
- Apply the driver-class acceptance checklist in `indigo_test/DRIVER_TESTING_RULES.md`. Map supported behavior and failure modes to concrete tests, and justify every non-applicable, hardware-only, or otherwise uncovered case in the driver's `REFACTOR.md`.
- Run strict builds and warnings checks, sanitizer-backed tests, and tests on the operating systems and architectures relevant to the driver's declared support when the environment provides them. Record commands, results, unavailable environments, and residual coverage gaps in the driver's `REFACTOR.md`; do not present unavailable platform testing as successful validation.

## Running firmware on an ESP32-S3

- Store only the firmware `.bin` and deployment notes in the driver's simulator directory. Do not store firmware source code or source archives.
- Deployment notes must identify the upstream project, firmware version or commit, ESP32-S3 adaptations, image checksum, flash layout, pin configuration, and serial baud rate. Verify that distributing the binary complies with its license; omitting source does not remove source-distribution obligations.
- Resolve firmware paths relative to the simulator directory. Never hard-code absolute paths.
- Flash or reset the board only when explicitly authorized. Confirm that no other test or application is using it. Never interrupt an existing test.
- Identify the board's serial port; do not assume the first available port is correct.
- Locate an installed Python environment with `esptool` and `pyserial`. Verify the chip type and flash capacity using `esptool`. This may reset the board and requires exclusive access.
- Verify the image checksum and deployment notes before uploading.
- For a **merged image** containing the bootloader, partition table, and application, flash at offset `0x0`:

  ```sh
  python -m esptool --chip esp32s3 --port "$ESP32_PORT" \
    --baud 460800 write_flash 0x0 "$FIRMWARE_BIN"
  ```

  `FIRMWARE_BIN` must be a relative path. Follow the syntax supported by the installed esptool version.

- Never flash an application-only image at `0x0`. Follow its documented offsets.
- Do not erase the entire flash unless explicitly authorized.
- Require successful upload verification, then allow the board to reboot. Use BOOT/RESET only if automatic download entry or reboot fails.
- Open the documented serial interface and baud rate. The prepared UART builds use UART0 through the USB-to-UART bridge, with native USB CDC disabled.
- Verify identity and basic protocol responses first. Without motors or sensors, report only communication and firmware state validation.
- Close the serial verification connection before connecting INDIGO. Run only authorized hardware scenarios.
- Finish by stopping commanded motion and tracking, closing serial connections, and recording the image checksum, board identity, upload result, test results, and firmware left installed.

## Testing mounts with MountSim

- MountSim acceptance requires macOS, a logged-in GUI session and a built MountSim application. Read the MountSim checkout's `README.md` and build it before testing. Record the app version and selected model returned by its control server.
- Keep these platform-specific tests under `indigo_test/mountsim/`, separate from the portable integration suite and physical-hardware tests. Guard their Makefile rules with `OS_DETECTED=Darwin`; never add them to `test`, `test-integration`, or Linux/Windows build and test dependencies.
- Reuse `indigo_test/mountsim/run_mountsim.py` and `mountsim_test_common.h` for every mount family. Put driver-specific cases in `test_<exact_driver_name>_mountsim.c` with an explicit opt-in Makefile target. Register new persistent files in the Xcode project. See [MountSim harness usage](../indigo_test/mountsim/USAGE.md) for the launcher contract, arguments and artifact layout.
- Build the production driver, then run its MountSim target. For Temma, from the INDIGO repository root:

  ```sh
  make -C indigo_drivers/mount_temma -f ../../Makefile.drv all
  make -C indigo_test test-mount-temma-mountsim
  ```

  The default app path points to the sibling MountSim checkout's Debug build. Set `MOUNTSIM_APP` to an absolute `.app` path when using another location.

- Let the shared launcher own each test's application instance, private control port, isolated preferences, PTY relay and watchdog. Run cases serially, each in a fresh process and app instance. Use `indigo_test_use_private_home()` before starting the INDIGO bus. Do not reuse or stop a user's running MountSim session or alter their saved settings.
- Select the exact case-sensitive model identifier accepted by MountSim's control server. Connect the real driver through `DEVICE_PORT` using the PTY supplied by the harness. The relay must forward bytes unchanged; trace framing must never translate commands or fabricate mount replies. Choose a trace terminator appropriate to the protocol, or use raw capture for binary protocols.
- Apply the mount and guider acceptance rules in `indigo_test/DRIVER_TESTING_RULES.md`. Check fresh property transitions, actual coordinate readback and motion completion, reachable GOTO/SYNC, manual motion, tracking/rates, park/abort, driver-specific settings, shared-device lifecycle and reconnect. An accepted command or OK property alone is not proof of arrival. Choose reachable targets relative to the configured site and current sidereal time rather than relying on a fixed sky position.
- Exercise transport loss through the shared relay disconnect/reconnect helper and apply the returned new PTY path before reconnecting the driver. Establish a healthy idle poll before testing idle loss. Distinguish client transport loss from an application crash, electrical serial faults and physical-device removal.
- Measure guiding through recorded direction ON/OFF command edges with the durations, repeats, warmups and workloads required by the guider rules. Label these results as software transport timing, including host/relay scheduling; do not describe them as physical relay or motor timing.
- When a discrepancy appears, preserve a reproducer and wire logs, and cross-check the protocol against multiple independent public sources, preferably manufacturer documentation and established implementations. Determine whether the defect belongs to the driver, MountSim, the harness or another component. Record sources, disagreements, root cause and regression evidence in the driver's `REFACTOR.md`. Fix the responsible driver or MountSim code within the authorized test mode; never change the emulator merely to agree with an incorrect driver. Add portable regression coverage for driver defects where possible, without introducing a MountSim dependency into that suite.
- After a production fix, rerun the full requested MountSim scope and the relevant portable regression suite. Preserve useful captures before `make -C indigo_test test-clean`; the default logs under `indigo_test/build/mountsim-results` are removed by that cleanup. Stop only app/test processes owned by the harness and close their transports.
- Record each run in the driver's `README.md` Testing section using `MountSim <version> (<model>)` as the type, for example `MountSim 2.3 (Temma)`, rather than `simulator`. Follow the existing timestamp, driver-version, platform and result format, and regenerate `TEST_SUMMARY.md`. Keep MountSim case counts separate from portable integration and physical-hardware counts in `REFACTOR.md`; do not add macOS-only cases to `MIGRATION_STATUS.md`'s portable integration or hardware counts. Do not claim physical hardware or Linux/Windows validation from a MountSim run.

## Generated drivers and repository integration

- When a driver uses `indigo_generator`, preserve the `.driver` file as the source of truth and follow all generated-driver rules in the root `AGENTS.md`. Make behavioral edits in `.driver`, regenerate the checked-in outputs, and verify reproducibility by regenerating and confirming that no unexplained diff remains.
- Add every new persistent file, including `REFACTOR.md`, `.driver` inputs, simulators, fake SDKs, and tests, to the appropriate build systems and project files required by the repository rules. Do not add temporary files, generated build products, or test artifacts.
- Do not repurpose or modify a user-facing driver `README.md` as a technical refactoring log. Put audit findings, decisions, limitations, migration progress, and verification evidence in `REFACTOR.md`; change a `README.md` only with explicit user approval as required by the root instructions.

## Mandatory workflow checklist

- When a user asks to continue or complete a driver refactoring, do not end the task at an intermediate buildable state or after a partial test pass. Continue through every applicable planned step, status update and final verification unless an external dependency or a required user decision genuinely blocks progress.
- After every change to a driver's integration or opt-in hardware tests, recalculate and update that driver's `Automated Tests (Sim / HW)` counts in root `MIGRATION_STATUS.md`. The first number is the hardware-free integration-case count and the second is the physical-hardware-case count; preserve the Comment column exactly.

1. Read the applicable instructions, driver sources, protocol/SDK documentation, build files, simulator/fake SDK, and tests.
2. Create or update the driver's `REFACTOR.md` before production-code changes.
3. Record the complete current-state audit and the baseline build/test evidence.
4. Record the explicit hardware-test decision, device/model when applicable, and validation plan.
5. Write the atomic migration, simulator/fake-SDK, test, integration, generation, and verification plan.
6. Before any production or generator-input change, complete and audit the required simulator or fake SDK and full applicable automated coverage against the original driver, including failure, lifecycle and concurrency scenarios; make all preservation tests pass and record any dedicated expected-failure defect reproducers.
7. Capture the normalized original-driver reference trace, record its command and result in `REFACTOR.md`, and run the original-driver hardware baseline when compatible hardware is available.
8. Execute one atomic migration step at a time; run the characterization suite and compare its reference trace after every behavior-changing step, then record the result and every intentional difference before proceeding.
9. For every exposed guider interface, run and record the required guiding-pulse duration accuracy measurement in `REFACTOR.md`.
10. Run the relevant generation/reproducibility checks, strict builds, warnings checks, sanitizers, tests, hardware comparison, and supported-platform/architecture validation available in the environment.
11. Update MIGRATION_STATUS.md when needed.
12. Perform a final audit of the diff, driver versioning, generated outputs, project/build registration, test/build artifacts, found-defects record, reference-trace comparison, and repository-required migration and test documentation. Add the separate simulated and hardware test run/pass totals to the end of `REFACTOR.md`, reconcile all status documents with its evidence, remove avoidable temporary artifacts, and leave no completed claim unsupported.
