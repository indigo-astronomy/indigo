# Driver Refactoring Rules

## Scope and precedence

- These instructions apply to every driver subtree under `indigo_drivers/`.
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

- Name every driver-specific automated-test source and executable target with the exact driver name, for example `test_aux_dsusb_sdk.c` and `test_aux_dsusb_sdk`. Do not place driver coverage in generic shared test sources such as `test_usb_outputs.c`; shared helpers may contain reusable harness code, but each driver's registered cases must live in its conventionally named test file.
- For every serial or hidapi driver, create or maintain a deterministic protocol simulator and complete applicable simulator-backed coverage. Cover the happy path, command and reply handling, malformed replies and transport/protocol failures, lifecycle and reconnect, and abort, overlapping operations, timing, and race behavior when relevant.
- For every SDK-based driver, create or maintain a fake SDK and complete applicable fake-SDK-backed coverage. Cover successful SDK behavior, SDK errors, lifecycle, hot-plug and multiple devices, and concurrency, races, and timing when relevant.
- For every driver that exposes a guider interface, whether standalone or as an additional logical device of a camera, mount, AO, or other driver, run a guiding-pulse duration accuracy test and record the result in that driver's `REFACTOR.md`. Follow the Guider timing measurement rules in `indigo_test/DRIVER_TESTING_RULES.md`; state the requested durations, measured endpoints and workload, sample count, signed and absolute error statistics, and whether the measurement covers simulator/fake-SDK software timing or physical relay output. Never present simulator, SDK-entry, or public-property completion timing as hardware pulse accuracy.
- If a simulator or fake SDK already exists, audit it against the documented protocol or SDK and extend it to close identified gaps. Preserving only an existing smoke test is insufficient.
- Apply the driver-class acceptance checklist in `indigo_test/DRIVER_TESTING_RULES.md`. Map supported behavior and failure modes to concrete tests, and justify every non-applicable, hardware-only, or otherwise uncovered case in the driver's `REFACTOR.md`.
- Run strict builds and warnings checks, sanitizer-backed tests, and tests on the operating systems and architectures relevant to the driver's declared support when the environment provides them. Record commands, results, unavailable environments, and residual coverage gaps in the driver's `REFACTOR.md`; do not present unavailable platform testing as successful validation.

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
6. Execute one atomic step at a time; verify it and record its result beside that step before proceeding or claiming completion.
7. Complete and audit the required simulator or fake SDK and full applicable automated coverage, including failure, lifecycle, and concurrency scenarios; for every exposed guider interface, record the required guiding-pulse duration accuracy measurement in `REFACTOR.md`.
8. Run the relevant generation/reproducibility checks, strict builds, warnings checks, sanitizers, tests, and supported-platform/architecture validation available in the environment.
9. Update MIGRATION_STATUS.md when needed.
10. Perform a final audit of the diff, driver versioning, generated outputs, project/build registration, test/build artifacts, found-defects record, and repository-required migration and test documentation. Add the separate simulated and hardware test run/pass totals to the end of `REFACTOR.md`, reconcile all status documents with its evidence, remove avoidable temporary artifacts, and leave no completed claim unsupported.
