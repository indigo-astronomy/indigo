# Refactoring plan for INDIGO 3.0 LACERTA Motorfocus driver

Goal: migrate `focuser_lacerta` to generated INDIGO 3.0 code, preserving the single focuser interface and supported controls while making serial transactions, motion completion and teardown reliable. Follow the staged approach in `aux_mgbox/REFACTOR.md` and the focuser-specific guidance in `focuser_asi/REFACTOR.md`.

Status (2026-09-09): Steps 1–7 completed. Authoritative generated source, simulator and applicable focuser-standard tests are synchronized. Final universal macOS build and 43/43 native arm64 scenarios pass; 19 targeted driver-instrumented ASan scenarios pass. Hardware, Windows/Linux builds and x86_64 runtime remain unverified. Historical baseline/intermediate results below are preserved.

## References and scope

- Root `AGENTS.md`, `indigo_test/AGENTS.md`, common and Focuser Driver Test Standard in `indigo_test/DRIVER_TESTING_RULES.md`.
- `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `DRIVER_DEVELOPMENT_BASICS.md`, `DEVELOPMENT.md`, `MAKEFILES.md`, `SERIAL_DEVICE_SIMULATORS.md`; repository `README.md` and `TESTING.md` for build/platform and hardware validation conventions.
- This folder's README, driver C/header/main and host simulator; `indigo_test/integration/test_focuser_lacerta_simulator.c`, shared serial harness and Makefile.
- `Lacerta_Mfoc-Fmc_API_2024.xlsx`, sheet `Munka1`, read directly during preparation. This is the primary bundled protocol reference. The simulator and driver are supplementary implementation evidence.
- `indigo_libs/indigo_focuser_driver.c` supplies inherited property defaults; `indigo_docs/PROPERTIES.md`, section `focuser_lacerta`, supplies the source mapping.
- `indigo_drivers/REVIEW.md` lists this driver in the reviewed scope but currently has no dedicated Lacerta finding. Its last-reviewed baseline is `017ba602857378e4aed489c065c76eacae15924c`. This preparation is not a folder-wide incremental review; do not advance review baselines. Record confirmed fault findings/dispositions there during implementation and keep test coverage in `indigo_test/CHANGES.md`.
- Temperature compensation, motor-current settings, home and other currently unexposed protocol features are outside this migration. Do not add them merely because the workbook documents them. No new TCP protocol or hot-plug support is planned.

## Baseline and public contract

- Baseline commit: `d531bce26895e98ba336807472165e1fcb8e5d6d`.
- Initial workspace has a pre-existing modification to root `MIGRATION_STATUS.md`; preserve it.
- Current version `0x02000001`; entry point/name `indigo_focuser_lacerta`; label `LACERTA Motorfocus Focuser`; logical name `LACERTA Motorfocus`.
- One logical focuser per serial connection, optional additional instances, no shared sibling device and no hot-plug. Model identity accepts `FMC` and `MFOC`, but the current implementation can also keep an UNKNOWN model connected. Model-specific behavior is otherwise not implemented.
- Legacy `indigo_open_serial()` uses 9600 baud. Port/list are visible; macOS selects the first `/dev/cu.usbmodem*`, Linux defaults to `/dev/usb_focuser`. Generated serial defaults must be inspected and any intentional selection change documented; do not retain platform boilerplate by default.
- README calls the driver platform independent but status Untested. Root migration status currently records API 2, no Windows, generator or async queues, no retest, and an automated test exists. No Windows project exists in this folder.
- All properties are inherited; there are no driver-local `indigo_init_*_property()` or `indigo_init_*_item()` calls and no custom property names. Future custom names, if explicitly needed, must start with `X_`.

### Property inventory and intentional corrections to assess

| Property | Current specialization / migration contract |
| --- | --- |
| `DEVICE_PORT`, `DEVICE_PORTS` | Visible before connection; fixed 9600 baud transport. Use generated serial ownership. |
| `INFO` | Six items; identity and firmware read on connect. Require valid replies before reporting successful detection. |
| `ADDITIONAL_INSTANCES` | Visible on the base instance only. Use generator/framework defaults and verify two independent PTYs. |
| `FOCUSER_POSITION` | Inherited RW GOTO/SYNC; preserve measured value while changing target. No explicit position-range specialization in the driver. Audit framework limit propagation before correcting device-supported bounds. |
| `FOCUSER_STEPS` | RW, 0–250000, step 1; converts relative inward/outward plus reversal into an absolute `M` command. Uses current position and clips against position metadata, which must agree with actual device limits. |
| `FOCUSER_DIRECTION`, `FOCUSER_ON_POSITION_SET` | Inherited switch behavior; GOTO/SYNC selector explicitly visible. Avoid duplicating inherited copy-only dispatch. |
| `FOCUSER_ABORT_MOTION` | Sends `H`, checks reply, but motion target/state and abort-switch completion need explicit acceptance tests. |
| `FOCUSER_LIMITS` | Visible, two items. Minimum fixed at 0; maximum 300–250000, default 250000, read/set with `g`/`G`. Workbook limits v1 to 65535. Resolve capabilities from identity/firmware and update metadata rather than adding generic input checks. |
| `FOCUSER_BACKLASH` | Visible inherited 0–999, step 0; `b`/`B` read/set. Workbook specifies integer 0–255: correct metadata during the behavior-fix step and document this intentional change. |
| `FOCUSER_REVERSE_MOTION` | Visible; `r`/`R` read/set. Verify both hardware flag readback and relative-command direction, without assuming simulator motion models physical reversal. |
| `FOCUSER_TEMPERATURE` | Visible RO, polled with `t`. Workbook value 99.9 means sensor not connected; do not publish it as valid temperature. |
| `FOCUSER_SPEED` | Explicitly hidden, no hardware command. |
| `FOCUSER_COMPENSATION`, `FOCUSER_MODE` | Remain inherited/hidden; temperature compensation is explicitly unsupported in README. |

No custom persistence handler exists; persistence follows the base focuser. Inspect actual base CONFIG handling before introducing any `persistent` override. Connected-only controls must remain connected-only. Avoid redundant `on_attach` state assignments when connection initialization already sets the state before publication.

## Protocol evidence and simulator gaps

Workbook `Munka1`:

- Rows 5–12: backlash 0–255 and intervening `D` debug messages before the final `b` response.
- Rows 17–18: maximum position 300–250000, v1 maximum 65535.
- Rows 19–22: halt reply `H 1`, identity `i MFOC`/`i FMC`, absolute move and completion replies `M(position)` plus `p(position)`.
- Rows 25–29: current-position query `q` returns `p`; `P` changes the coordinate without moving; reversal flag is 0/1.
- Row 32: temperature 99.9 is NC. Row 35: firmware forms v1/v2/v3. Rows 46–49: FMC v1.1.xx and MFOC v2.1.xx/v3.1.xx.

The workbook documents command/reply text but not every byte-level detail. Current code/simulator supply CR framing and whitespace variants; retain them as supplementary evidence and verify on hardware later. Do not fabricate acknowledgements for commands whose completion is asynchronous.

The host simulator already supports `--headless`, `--ready-file`, `--trace`, `--model MFOC|FMC` and `--firmware`. It defaults to MFOC 3.1.123, position 0, max 250000, backlash 3, temperature 23.5. Initially motion advanced one step per worker-thread millisecond; the follow-up below replaces this with shared elapsed-time motion at 1000 steps/s and a 0.5-second minimum nonzero duration. It implements the driver's current command subset, but has no fault profiles, debug interleaving, motion completion messages or firmware-specific range restrictions. Reverse is stored/read back; its physical meaning is not modeled. The baseline integration suite registered only one default-model scenario; two direct protocol cases were added in the follow-up below.

## Implementation audit checkpoints

These are source observations for the planned fixes; fault injection is still pending, so no runtime failure or ASan reproduction is claimed here.

- `lacerta_command()` (C lines 59–104) permits `index == max` before writing the terminator. Reserve terminator capacity and consume/recover from overlong frames. Its 100-line retry loop can take roughly 50 seconds on silent input, ignores the write result, and returns true even when the expected response never arrived. Replace with an overall bounded transaction deadline and exact reply validation.
- `lacerta_open()` (lines 106–145) uses integer descriptors and `> 0`, accepts unvalidated response offsets and UNKNOWN identity, and does not require successful firmware/initial settings reads. Make acquisition transactional and validate each required stage; test rollback and reconnect.
- Motion polling (lines 153–177) only completes state when the measured position changes. A no-op or clipped target can remain BUSY; abort can leave an unreachable old target. Determine completion from validated readback and operation ownership, including a no-op, external movement and a new move after abort.
- One polling timer is tracked; change handlers are separately scheduled with untracked zero-delay timers. A port mutex serializes transactions but does not by itself own pending-handler cancellation during close. Move all device I/O onto the generated device queue and remove mutex/timer scaffolding only once all callers are migrated.
- Setters use unchecked `atoi`/`indigo_atod` and often treat a matching first byte as success. Verify framing, full numeric conversion, reply range and requested-versus-returned setting; reject bad device data without overwriting accepted values.
- Private `type_property[2]` is unused; remove it rather than carrying it into generated data. Retain only actual model/capability and operation state.
- Narrow test target does not depend on the production archive. Add that prerequisite during test preparation; until then force relink after rebuilding production. Existing cleanup only calls driver teardown when `context.connected`; harden failed-connect cleanup before adding failure scenarios.

## Atomic implementation plan

Each step is a separately reviewable change with its own result recorded here. Keep failing regressions explicit until their corresponding fix; do not convert them to expected passes.

1. **Establish protocol and failure fixtures.** Use shared `serial_motion` for simulated movement (completed in the follow-up below). Extend the existing simulator with bounded, selectable debug/interleaved/fragmented/overlong/missing/malformed responses, temperature NC and documented firmware/model variants. Add observable commands and motion state using scoped trace/event instrumentation. Correct asynchronous `M`/`p` completion behavior against the workbook. Independently check the simulator, then add named public-bus tests, reliable cleanup/watchdogs, archive dependency and a driver-instrumented ASan target. Preserve the existing passing smoke test. Record new coverage/gaps in `CHANGES.md` and formal reproduced findings in driver `REVIEW.md`.

2. **Make serial transactions safe and portable.** Move to `indigo_uni_handle`, exact `lacerta_open(indigo_device *device)`/`lacerta_close(indigo_device *device)` names, bounded CR framing/deadline and complete write/reply validation. Skip documented debug/completion traffic without confusing it with the requested reply. Roll back all resources on failed open/identity/initialization. Increment the driver version. Test both models, invalid port, unknown/silent identity, each failed initial query, short/overlong/nonnumeric replies, timeout then valid recovery and repeated reconnect; run targeted ASan.

3. **Correct readback and focuser semantics.** Apply documented model/firmware ranges and NC-temperature handling. Make absolute/relative motion, SYNC, no-op, limit-clipped motion, external position changes and abort produce correct measured values, targets and BUSY/OK/ALERT. Confirm backlash/reversal/limits readback, including mismatches and partial initialization failure. Handle operational cross-property conflicts without duplicating generic framework validation. Increment version for behavior fixes. Test command values/order and fresh property transitions, both directions/reversal, abort idle/moving then new move, start/poll/stop failures and recovery.

4. **Serialize lifetime and operations on handler queues.** Replace timer-dispatched changes and the mutex with device-queue operations and bounded polling. Use named motion finalizers where delayed operation completion is required; periodic temperature/idle polling remains polling. Cancel identified pending motion on abort and all owned pending work on disconnect. Test disconnect during motion/read, overlap through POSITION versus STEPS, close ordering and reconnect. Keep this ownership change independently testable; if generated lifecycle makes steps 4–5 inseparable, explain the coupling and validate the combined transition rather than maintaining two divergent implementations.

5. **Introduce authoritative `.driver` and generate lifecycle.** Declare `driver lacerta`, `serial;`, one `focuser` named `LACERTA Motorfocus`, additional instances and inherited property specializations. Prefer generator attributes/defaults over boilerplate; use `connection_result` without returning from connection hooks. Let generated open/close, allocation, dispatch, enumeration, attach/detach and queue cleanup own their work. Use `preserve_values` for measured position as appropriate. Retain no duplicate busy guards, standard completion updates or `MAX_DEVICES` override. Preserve license, extend copyright through 2026 and add the Codex refactoring notice in source. Raise generated version above the latest handwritten version. If extracting, use `indigo_generator -c indigo_focuser_lacerta.driver`, preferably in a temporary directory, never the existing C as output. Generate C/header/main together, inspect each handler/finalizer and run the entire narrow suite.

6. **Verify ownership and independent instances.** Exercise each supported model/firmware branch, failed initialization/reconnect, disconnect during pending operations, and two independent PTYs with distinct position/temperature/limits. Confirm the peer survives disconnect and no parser, timer, target or model state leaks. Verify inherited visibility and supported metadata, no commands after close and clean INIT/SHUTDOWN. Run the final ordinary suite and targeted ASan for parser/lifetime cases. Use strict warning checks for both macOS architectures, including `-Wconversion`, `-Wshorten-64-to-32` and `-Werror`; do not repeat the MGBox warning gap.

7. **Synchronize projects and acceptance records.** Add `.driver` and `REFACTOR.md` to the relevant Xcode group. Add Windows project/filter/solution integration using a comparable generated serial focuser and portable I/O; distinguish source portability/project availability from an actual Windows build/run. Update README and `PROPERTIES.md` for generated source, corrected ranges, visibility and failure/completion behavior. Update test coverage notes and only verified review dispositions without advancing a partial review baseline. Update root `MIGRATION_STATUS.md` when migration is complete, preserving the initial user edit and recording simulator versus hardware/platform validation. Regenerate twice and compare outputs byte-for-byte, review the complete diff, check version increase and `git diff --check`, clean tests and reap all owned simulators. Record hardware/other-platform gaps explicitly.

## Baseline validation results (2026-09-09)

- Forced production C/main compilation and archive/dylib/executable linking passed for macOS x86_64 and arm64 with the normal Makefile flags.
- Simulator and forced-relinked integration test builds passed for both architectures. Native macOS arm64 execution: the single `lacerta_focuser_passes_serial_compliance_checks` scenario passed (exit 0), covering connect, property enumeration, SYNC to 1000, backlash 5, reversal enabled, maximum 200000, GOTO to 1200 and normal cleanup.
- This is a smoke baseline, not full focuser-standard coverage. No explicit relative move, abort, failure, alternate model, no-op, limit boundary, external motion or instance-isolation assertion currently exists. Waiting on cached OK without an event revision also needs strengthening in expanded tests.
- Strict syntax check of the unchanged driver exposed five existing conversion warnings: command length signedness, three motion/position conversions and the preserved-position copy. No ASan or malformed-frame runtime test was run during preparation.
- Production/simulator/test sources and DRIVER_VERSION remain unchanged. No generator was run and no migration-complete status is claimed. Test artifacts were removed with `make -C indigo_test test-clean` after the baseline; no hardware, Linux/Windows or x86_64 runtime validation is claimed.

Commands from the repository root:

```sh
make -C indigo_drivers/focuser_lacerta -f ../../Makefile.drv -W indigo_focuser_lacerta.c -W indigo_focuser_lacerta_main.c
make -C indigo_test -W integration/test_focuser_lacerta_simulator.c build/integration/test_focuser_lacerta_simulator
```

Run from `indigo_test`:

```sh
./build/integration/test_focuser_lacerta_simulator
```

Cleanup from root: `make -C indigo_test test-clean`.

## Completion criteria

Full applicable coverage is mandatory for every refactoring, not only the existing smoke scenario. Map every supported capability and failure/lifecycle branch from the focuser standard to named tests in `CHANGES.md`; mark genuinely non-applicable or hardware-only checks with reasons. Use the bundled XLSX as the primary protocol oracle. Migration is not complete while applicable simulator-backed cases remain unimplemented or failing.

Authoritative and reproducible `.driver`/C/header/main, generated queue/lifecycle ownership, portable bounded I/O, increased version and no strict compiler warnings. Simulator-backed evidence must cover the exposed focuser behavior and failure/lifetime cases above, with explicit protocol provenance. Project files, property/README documentation, test notes, this plan and `MIGRATION_STATUS.md` must agree. Hardware acceptance remains a separate small-travel check of move/SYNC/abort/settings and reconnect on available FMC/MFOC hardware; no optical focus or travel to mechanical stops is required.


## Step 1 partial result: shared simulator motion (2026-09-09)

- Replaced the simulator's one-step-per-sleep background thread and mutex with `serial_motion.h`. Position queries update elapsed-time motion; M starts clipped absolute movement at 1000 steps/s, H stops at the current measured position and P synchronizes/cancels the previous motion. Short nonzero moves now use the shared helper's 0.5-second minimum, an intentional test-observability change rather than physical motor timing. Existing wire command/reply forms remain unchanged; unsolicited completion/debug traffic remains a later fixture task.
- Added shared-header build dependencies in both simulator Makefiles, removed the standalone pthread link dependency and added the production archive prerequisite to the integration target. The host simulator has no INDIGO DRIVER_VERSION; production version remains `0x02000001` because production behavior did not change.
- Added `lacerta_simulator_mfoc_protocol` and `lacerta_simulator_fmc_protocol`, covering the entire currently implemented command subset: identity/version, temperature, backlash/reversal/limit write and readback, query/SYNC, elapsed-time movement in both directions, halt idle/moving, SYNC during motion, no-op, clipping at both ends and observable short motion. Numeric settings use XLSX-supported values. Out-of-travel M commands test the simulator's existing clipping contract directly, not generic framework validation through the driver.
- These two direct simulator cases are independent fixture validation, not full production-driver coverage. The original public-bus scenario remains. Full model-specific limits, temperature NC, debug/asynchronous completion, malformed/failed transactions, driver motion/abort/overlap and lifecycle/instance tests are still required by the remaining plan.
- Added the repository rule to `AGENTS.md`: every refactoring requires full applicable driver-standard coverage and a simulator audit against primary documentation, with shared `serial_motion` for representable serial motion. No generator or shared motion-helper implementation was changed.

- Validation: simulator/integration universal macOS build passed; simulator strict `-Wall -Wextra -Werror -std=c11` syntax check passed. All three registered cases passed on native arm64, including the unchanged production driver. The first direct protocol run exposed a test-helper mistake: `indigo_uni_read_section` retained CR because CR was not in the trim set; corrected the observer to trim CR/LF and reran successfully. No protocol or production fix was used to hide that test failure. Standalone simulator build also passed. `git diff --check` passed and `make -C indigo_test test-clean` removed test artifacts. No ASan, hardware or other-platform execution is claimed for this partial step.


## Steps 1–2: transaction regressions and first fix (2026-09-09)

The simulator now has documented asynchronous M/p completion, optional debug/split framing, firmware v1 maximum, NC temperature, a private command journal and one-shot out-of-band fault controls for individual transactions. The test parent owns the PTY and a 35-second child watchdog, and always reaps the simulator. An ASan target instruments the actual production driver.

Baseline expanded run: normal passes; no-op motion, unknown/short/silent identity fail, overlong identity aborts. ASan confirms a stack-buffer-overflow in `lacerta_command` when terminating the full 32-byte response. Two independent simulator cases initially also failed because their observer did not skip newly implemented asynchronous M completion before p; this is a test-observer issue, corrected separately from production fixes.

Handwritten version `0x02000002` replaces descriptor I/O with uniform handles, bounds framing and stale-data drain, uses a two-second transaction deadline and checks full writes/identity/firmware. All four identity rejection cases then passed, and the overlong identity ASan reproducer passed. No-op/motion-state repairs and required initial numeric readback checks continue in the generated transition. No generator changes were made. The queue and motion fixes of Steps 3–5 are integrated in the authoritative DSL to give delayed completion one lifecycle owner; no second handwritten motion implementation will be retained.


## Steps 3–5: measured state, serialized operations and generated source (2026-09-09)

Completed together as permitted by Step 4: generated queues own motion finalizers, polling and disconnect, so introducing a temporary second handwritten queue implementation would duplicate lifecycle ownership. `.driver` is authoritative; C/header/main are generated without changes to the generator. The final version is `0x03000005`, above baseline `0x02000001` and the intermediate handwritten transport fix `0x02000002`.

- Validated initial reverse, maximum, position and backlash readback; firmware v1 caps maximum at 65535, v2/v3 at 250000. Backlash is 0–255. Reported NC temperature preserves the last valid value and reports ALERT. Unsupported speed/mode/compensation stay hidden; inherited operational controls remain connect-scoped.
- Motion publishes measured value independently of target. GOTO/relative/no-op/clipping/SYNC and external position polling have explicit completion semantics. Relative direction honors reversal. Both cross-property overlap orders are rejected before target copying. Periodic idle polling cannot overwrite a queued motion target.
- Abort owns cancellation of the identified pending motion finalizer, checks H and q, resets its switch and publishes stopped position. A failed stop retains motion monitoring; uncertain stopped-position readback blocks a new movement until recovery. Poll failure attempts H and reports ALERT. One hundred unchanged motion polls trigger stop/ALERT (nominal 10 seconds, an explicit software policy).
- Settings use device-confirmed values, restore reversal on failure and reject changes during motion. Changed limits redefine POSITION/STEPS metadata. Generated standard handler prologues/epilogues are retained; actual `_finalizer` references in motion/abort handlers mean those handlers explicitly own BUSY/error/completion publication.
- Removed legacy mutex/timer-dispatched change scaffolding; generated per-device queues serialize I/O and teardown. No custom MAX_DEVICES, shared reference counter or persistent opened flag was introduced.
- Inspection found that the unchanged generator's single-device failed `on_connect` path does not close a successful low-level open. The DSL explicitly closes only on failed required initial readback, while `lacerta_open` rolls back its own failures. Descriptor-count regressions verify the ownership. Successful sessions close through generated disconnect. No generator modification was needed.

Intermediate verification was intentionally retained: first expanded generated run exposed stale simulator-observer input handling and missing limit metadata publication. The observer now drains input without `indigo_uni_discard` (which can discard pending output), and skips documented asynchronous completion. Driver limits use delete/define for changed ranges. The following generated run passed all 43 scenarios. Final version additionally validates bounded firmware numeric conversion and failed-initialization descriptor rollback.

## Step 6: final validation and coverage (2026-09-09)

- Production universal macOS arm64/x86_64 C/main compilation and archive/dylib/executable linking passed. Final ordinary suite passed **43/43 scenarios** on native arm64, including both POSITION/STEPS overlap orders and device settings after repeated reconnect.
- Final ASan executable instruments production driver C and the test (framework remains a normal library build). Filters `identity` (4), `init_` (5), `disconnect` (2), `instances` (1), `poll_` (7) passed **19/19** with no ASan finding. Initial ASan stack overflow reproduction is fixed.
- Strict generated C syntax checks passed for both architectures with `-Wall -Wextra -Wno-unused-parameter -Wconversion -Wshorten-64-to-32 -Werror`. Standard builds emitted no warning.
- Named full applicable common/focuser-standard mapping is in `indigo_test/CHANGES.md`, “LACERTA generated migration: final coverage”: fixture commands; all model/firmware branches; property visibility/ranges; absolute/relative/no-op/limits/SYNC; direction/reversal; abort and recovery; settings failures; malformed/partial/flooding replies; temperature and external position; initialization/descriptor rollback; disconnect/reconnect and independent instances.
- Non-applicable controls: speed, automatic mode, compensation, current, beep/heater and home are not exposed. Generic input validation/config storage belongs to the framework. PTYs do not deterministically force partial OS writes; full write count is checked and transport loss/ignored motion is exercised without inventing an M acknowledgement. This is applicable behavior coverage, not an exhaustive branch-coverage percentage.

Reproduction (repository root):

```sh
build/bin/indigo_generator indigo_drivers/focuser_lacerta/indigo_focuser_lacerta.driver
make -C indigo_drivers/focuser_lacerta -f ../../Makefile.drv
make -C indigo_test build/integration/test_focuser_lacerta_simulator build/integration/test_focuser_lacerta_simulator_asan
```

Run from `indigo_test`:

```sh
./build/integration/test_focuser_lacerta_simulator
for filter in identity init_ disconnect instances poll_; do
  LACERTA_TEST_FILTER="$filter" ./build/integration/test_focuser_lacerta_simulator_asan || exit 1
done
```

## Step 7: projects, records and acceptance (2026-09-09)

Added DSL/refactoring notes to the Lacerta Xcode group; `plutil -lint` passes. Added Windows project/filter and Debug/Release ARM64/x64 solution configuration using the existing serial-focuser project pattern; XML parsing and references checked. This is project integration, not a Windows build claim.

README, property-source/range documentation, simulator inventory, test coverage and root migration status are updated. The pre-existing `ccd_mallin` status edit is preserved. DRV-098/099/100 are recorded as fixed against final simulator/ASan evidence without advancing the folder review baseline. The repository-wide full-coverage/simulator-audit rule is in AGENTS.md; the existing mandatory migration-status update rule remains in force.

Two consecutive generator runs reproduce C/header/main byte-for-byte. Final source version increase and absence of MAX_DEVICES overrides were checked. Hardware acceptance remains the class-standard small-travel move/SYNC/abort/settings/reconnect check on available FMC/MFOC hardware; physical motor timing and sensor accuracy were not measured. Linux/Windows and x86_64 runtime are unverified.

Final hygiene: `git diff --check` passed; `make -C indigo_test test-clean` removed the test build tree. Test parents reaped their simulators, and task-owned scratch source/log files were removed. No commit was created.


## Follow-up: Xcode conditional-initialization warnings (2026-09-09)

Reproduced three `-Wconditional-uninitialized` warnings for `position`, `backlash` and `maximum` in connection initialization. Xcode enables this through aggressive uninitialized-variable checking; the earlier strict command did not include this option, so its warning-free result was insufficient for the Xcode configuration. The compiler does not infer that the saved `connection_result` being true means every short-circuited query wrote its output.

Initialized all four local readback variables in the authoritative `.driver` and regenerated C/header/main. Required query validation and failure rollback are unchanged; zero initialization does not permit a failed query to connect. This is a warning-only correction, so version remains `0x03000005`.

Validation: universal production build passed. Strict syntax checks for arm64/x86_64 at both `-O0` and `-O2`, including `-Wconditional-uninitialized -Wunreachable-code -Wcomma -Wall -Wextra -Wconversion -Wshorten-64-to-32 -Werror`, passed without warnings. Six targeted simulator cases passed (`normal` and all five `init_` cases, including rollback/retry). `git diff --check` passed; test artifacts were removed with `make -C indigo_test test-clean`. Use the expanded warning flags for subsequent Lacerta checks.
