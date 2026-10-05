# Prodigy generated-driver migration

Status: complete, 2026-09-09. Clean starting tree; baseline version 0x02000002. No generator changes are authorized or needed. Preserve MIGRATION_STATUS Comment exactly.

## Protocol and audit

Primary source: bundled ProdigyMF-Serial-Command-Table.pdf, one page, firmware >= 1.4. Serial 19200 8N1, newline requests/replies. Identity is exactly OK_PRDG. A has ten colon-separated fields. B returns B:speed. M/G/W/S/C and power/USB setters echo the full command; H returns 0, Z returns Z:1, Q has no reply. D contains four booleans, including power outlet 2 (legacy driver/simulator incorrectly use 2). Reverse is fixed normal; no compensation support. Relative inward retains legacy positive G offset; hardware direction acceptance remains necessary. Z moves to the zero encoder; simulator will model elapsed-time zero motion, not instantaneous SYNC.

Legacy risks: shared handle overwritten on second logical connect; duplicate mutex initialization and timer cancellation while holding the same mutex; unchecked power/USB/park failures; unvalidated atoi/temperature/status parsing; relative response discarded; no enforced local limits; requested position overwrites measured position; polling-only simulator motion. No existing Prodigy-specific review finding was recorded.

## Atomic plan

1. Baseline build and both logical smoke tests. Audit PDF and replace simulator movement with serial_motion, add fault injection/journal and independent protocol fixtures; demonstrate regressions against baseline.
2. Author .driver with shared private receive buffer, bounded portable uni_io transport and strict complete reply validation. Generated master queue/shared handle reference counting and transactional initialization. Read actual speed via B. Regenerate, build and test initialization/parser paths.
3. Handler + motion/park finalizer, measured/target separation, local limits, SYNC/abort/failure recovery; powerbox readback, labels, reboot completion with bounded recovery and rejection during motion. Verify slave-first/shared lifetime and pending disconnect without custom generator scaffolding.
4. Full applicable focuser and powerbox tests: metadata/visibility, all exposed commands/readback, boundaries/directions/no-op, malformed/partial/timeout/rejection, overlap, park and reboot, simultaneous logical connections and independent instances. Strict O0/O2 arm64/x86_64 and targeted ASan.
5. Reproducible generation, project integration, README/PROPERTIES/simulator inventory, CHANGES mapping and scoped review dispositions; MIGRATION_STATUS status columns only. Final diff check and test cleanup; document all results here.

## Acceptance limits

No physical encoder, electrical ports or Windows/Linux execution can be certified by PTYs. Simulator protocol fixtures follow the supplied table; physical park travel and firmware-dependent reboot timing require hardware acceptance. No focus-quality/autofocus or framework numeric validation testing.

## Progress log

### Step 1 — baseline and simulator

Baseline universal macOS build passed and both original smoke scenarios passed (focuser and powerbox). Read and visually inspected the complete one-page PDF. Confirmed D outlet 2 is boolean 0/1, B supplies actual configured speed, and Z is encoder-zero motion. Existing simulator advanced only on I queries and reported the same incorrect second-outlet value as the driver.

Simulator now uses shared serial_motion with elapsed-time movement, stop and SYNC; park is asynchronous zero motion. Added B, split replies, a parent-owned command journal and one-shot malformed/partial/overlong/silent/ACK/frozen-motion/reboot fault injection. The legacy Arduino sketch remains unchanged. New direct protocol and production-driver regression scenarios are being built; final simulator validation is pending.

### Steps 2–3 — generated implementation draft

Created authoritative indigo_focuser_prodigy.driver and regenerated C/header/main. Version is 0x03000003 (baseline 0x02000002). Universal driver build passes. Generator owns master/slave queues and shared connection references; initialization rollback is verified in generated output. No generator implementation changes or MAX_DEVICES overrides.

Transport uses portable uni_io and one private-data 128-byte receive buffer, with bounded stale-input drain and a one-second complete-line deadline. Identity, status, numeric fields, port booleans and command acknowledgements are validated. Decoded scalars are saved before the next command overwrites the buffer. Initial speed comes from B. Long movement/park uses motion_finalizer; reboot uses reboot_finalizer with bounded retries. Local limits, measured versus target position, abort/recovery and partial port-write failures are handled explicitly. Powerbox interface no longer advertises weather.

### Step 4 — in progress

Expanded test draft contains 44 named scenarios covering both logical devices, protocol, motion/settings/park, fault handling, shared lifetime and reboot. Compiling the new harness now; these scenarios are not yet claimed as passing. Remaining work includes debugging tests/implementation, overlap and independent-instance coverage, strict warnings/ASan, baseline regression reproduction and final documentation/project/status updates.

### Intermediate verification

New harness initially collided with the system reboot() function name; renamed its test callback to reboot_case. The strict Xcode-style warning pass found conditional-uninitialized on the parsed speed local; initialized it in the DSL and regenerated. Strict O0/O2 driver compilation now passes for arm64/x86_64, and the simulator passes Wall/Wextra/Werror.

First expanded run passed protocol, initial capabilities and limits. Exact command-count checks caught the new simulator journal being appended across scenarios; changed each fresh simulator to truncate its own journal. This was a test-isolation defect, not a device regression. Queue inspection confirms indigo_cancel_pending_handlers removes only entries belonging to the requested logical device, so peer operations remain queued. Also ensured idle polling continues for externally observed motion and disconnect confirms stop/readback before clearing motion uncertainty.

### User-directed transport simplification

Compared focuser_dmfc's dmfc_command with uni_io implementation. Replaced the custom drain/read loop with indigo_uni_discard, indigo_uni_printf and indigo_uni_read_section2, retaining the private receive buffer. Unlike read_section's first-byte-only timeout, read_section2 bounds later bytes too (1 second first byte, 100 ms inter-byte). Keep LF in the result and verify it before stripping: partial/overlong/NUL replies are rejected. Pass capacity minus one because uni_io appends a NUL. Exact ACK/payload checks remain in protocol helpers. The maximum line length bounds total slow-stream time; no claim of a single one-second whole-line deadline after this simplification.

The first park test caught an early BUSY publication while PARK was still true. Reset the momentary switch before starting/publishing park; the finalizer still owns completion. Expanded coverage now includes overlap, failed slave initialization preserving its peer, reboot rejection during motion, disconnect during park/reboot and independent instances.

Transport simplification passes strict compilation. Baseline replay uses the original C/header in a temporary build: init_identity fails as expected because legacy code accepts OK_OTHER; generated driver rejects it and reconnects after the one-shot fault. No production file was reverted for this reproduction.

Further test refinement: the framework itself publishes a BUSY request before queued custom code runs, so a momentary switch can legitimately still be true in that first notification. Tests now wait explicitly for the handler's switch reset instead of racing that notification. This applies to park and reboot. Firmware validation now checks dotted digit strings instead of comparing floating-point versions (1.10 must not be mistaken for 1.1); added a dedicated firmware-minor fixture.

User-requested repository rule added to AGENTS.md: prefer indigo_uni_discard and indigo_uni_read_section2 when refactoring serial drivers, with explicit first/inter-byte timeouts, NUL capacity and protocol completeness validation; custom readers require a documented protocol need. Prodigy already follows this rule.

### Preferred-I/O audit requested during migration

Prodigy DSL and generated C contain no indigo_uni_wait_for_data or indigo_uni_read_available calls. Its direct protocol test still used read_line; converted it to read_section2 with explicit timeouts, NUL space and LF validation too. Earlier completed migrations still contain custom readers: focuser_lacerta (.driver:49–70), focuser_ioptron (.driver:43–60), binary focuser_efa (.driver:49–61), and the streaming MGBox parser (.driver:306–311). These are audit findings outside the current Prodigy patch; Lacerta/iOptron are candidates for the newly preferred delimited API, while EFA/MGBox need protocol-specific assessment.

### Coverage and project checkpoint

Expanded suite now has 53 scenarios; complete scenario-to-capability mapping is in indigo_test/CHANGES.md. All 23 selected ASan scenarios passed after the simplified production transport, covering parser/init faults, shared rollback/lifetime, disconnects, independent instances, transport loss, park abort and reboot timeout. The direct protocol test was subsequently converted from read_line to read_section2 and is rerun separately. Full ordinary suite is still running.

README, property documentation, simulator inventory and scoped DRV-105/106 dispositions are updated. User added Xcode DSL/REFACTOR references during work; preserved those edits. Added Windows project/filter and solution configurations. plutil, XML/file references and solution GUID/configuration checks pass. All three changed drivers (Prodigy plus requested Lacerta/iOptron follow-ups) pass strict O0/O2 arm64/x86_64 compilation with Wall/Wextra/conversion/shorten-64-to-32/conditional-uninitialized/unreachable-code/comma/Werror, and regenerate byte-identically. No generator edits or MAX_DEVICES overrides.

The user explicitly authorized fixing the Lacerta/iOptron audit findings after they were reported. Those two changes are now included, with private buffers and preferred APIs, version 0x03000006, and independent progress/results in their own REFACTOR.md files. EFA and MGBox are not changed by this follow-up.

Shared-I/O regression found during iOptron verification: indigo_uni_discard is documented to discard input, but its serial implementation used TCIOFLUSH / PURGE_RXCLEAR|PURGE_TXCLEAR, dropping pending output too. A readback query could therefore erase the preceding write-only Z/R/Q command. Corrected the shared implementation to TCIFLUSH / PURGE_RXCLEAR, preserving transmitted commands without adding platform code or timing sleeps to drivers. Existing zero/reverse/abort readback tests reproduce the defect; full suites are rerun with the corrected library.

The full movement scenario also caught missing preserve_values on FOCUSER_POSITION: the generated request could overwrite measured position with target before completion. Added the generator attribute in the DSL, regenerated and scheduled the full suite again.

User-directed variadic command pattern applied: command helpers accept format strings/arguments and use va_start/va_end with uni_vprintf (or uni_vtprintf for Prodigy newline termination). Numeric motion/settings arguments are formatted at send time. Repository rule added to AGENTS.md. Full tests are rerun on this final transport form plus the input-only discard fix.

Final variadic transport build checkpoint: all three drivers compile under strict O0/O2 arm64/x86_64 flags. Lacerta required explicit zero initialization of decoded position locals after separating command and response parsing; failed queries still return before publication. This warning-only refinement is separately smoke-tested after the full-suite run. The corrected input-only discard restores iOptron zero/reverse/abort readback regression cases.

Final verification runs include the variadic helpers and shared discard correction. Lacerta completed 43/43 + 19/19 ASan; iOptron completed 20/20 targeted ASan. Prodigy full/ASan runs continue; no failures have appeared in these final runs. Both follow-up drivers retain their simulator-tested status; no manual migration comments were edited.


## Final acceptance

All five planned steps are complete. Final Prodigy version 0x03000003 replaces baseline 0x02000002. Final run of all 53 simulator scenarios and all 23 targeted production-driver ASan scenarios passes with the variadic helper, preserve_values correction and input-only discard implementation. Strict O0/O2 arm64/x86_64 checks pass without warnings; generated C/header/main are byte-identical on regeneration. Xcode and Windows project validation passes. The two user-requested follow-ups also pass: Lacerta 43/43 ordinary + 19/19 ASan and iOptron 40/40 ordinary + 20/20 ASan, both version 0x03000006. Lacerta's subsequent warning-only initialization was additionally verified with normal and ASan motion_poll_failure.

MIGRATION_STATUS now records API 3, Windows project support, generator, queues, simulator retesting and automated tests for Prodigy. Comment is preserved exactly. Existing Lacerta/iOptron status columns remain accurate. User-added Xcode references and user commits made during this work are preserved. No generator edits or agent-created commits.

Reproduction from repository root:

```sh
make -C indigo_libs
build/bin/indigo_generator indigo_drivers/focuser_prodigy/indigo_focuser_prodigy.driver
make -C indigo_drivers/focuser_prodigy -f ../../Makefile.drv
make -C indigo_test build/integration/test_focuser_prodigy_simulator build/integration/test_focuser_prodigy_simulator_asan
cd indigo_test
./build/integration/test_focuser_prodigy_simulator
for io_filter in init_ poll_ shared disconnect instances transport_loss park_abort reboot_timeout; do
  PRODIGY_TEST_FILTER=$io_filter ./build/integration/test_focuser_prodigy_simulator_asan || exit 1
done
```

Windows/Linux execution, real relative polarity/encoder park, output electrical behavior and actual firmware reboot timing remain hardware/platform acceptance gaps. PTY and driver ASan tests do not certify those. The shared library is built normally; ASan instruments the production driver and test harness.

Final hygiene: all test parents exited and reaped their simulators; make -C indigo_test test-clean removed generated test artifacts. Task-owned temporary source copies, PDF rendering and diagnostic logs were removed after recording outcomes. Final git diff --check passes.

## Rejected-change regression coverage (2026-09-18)

Change requests refused by a busy guard are now declared with the generator's `reject_change` block. The generated guard marks every item for update, sets `INDIGO_ALERT_STATE` and publishes the property with the message, so the client receives the actual driver-side values instead of an `INDIGO_OK_STATE` update carrying no items, which left the refused value visible in the client.

Covered by the `rejected_change` scenario in `indigo_test/integration/test_focuser_prodigy_simulator.c`: while `FOCUSER_POSITION` is BUSY, both `FOCUSER_STEPS` and the `X_FOCUSER_PARK` switch end in ALERT with their values restored, no park command is issued, and motion is accepted again after the abort.

```sh
cd indigo_test && PRODIGY_TEST_FILTER=rejected_change ./build/integration/test_focuser_prodigy_simulator
```

## Coverage review (2026-09-20)

The suite was reviewed against the focuser class standard in
`indigo_test/DRIVER_TESTING_RULES.md` as part of the repository wide test coverage pass. It already
meets the standard: `indigo_test/integration/test_focuser_prodigy_simulator.c` runs 55 scenarios,
each in its own forked process against a freshly started simulator, and covers capability discovery
and firmware branches, connection rejection on every field of the identity and status replies,
lifecycle and additional instances, absolute and relative motion, limits, sync, park and its abort
and failure paths, refused overlapping requests, abort including the queued abort of a running
operation, stop and start failures, stalled motion, malformed, partial, overlong and silent poll
replies, temperature readback, the power box outputs and their failures, labels, the shared device,
reboot, disconnect during motion and transport loss.

No new scenario was added. The only change was recording the result, which was missing from
`MIGRATION_STATUS.md` and from the driver `README.md`.

```sh
make -C indigo_test build/integration/test_focuser_prodigy_simulator
cd indigo_test && ./build/integration/test_focuser_prodigy_simulator
```

- Simulated tests run: 55; passed: 55.
- Hardware tests run: 0; passed: 0.

## Switch target adoption: AUX_POWER_OUTLET and AUX_USB_PORT (3.0.0.8, 2026-09-27)

Findings TGT-013 and TGT-014 in `indigo_drivers/REVIEW_SWITCH_TARGETS.md`, both reproduced before the fix.

- **Defects (reproduced):** `reboot_finalizer`, on success, called `prodigy_publish_ports()`, which wrote the four port items from the ports it read back and published AUX_POWER_OUTLET and AUX_USB_PORT without a BUSY check. Finalizers are `INDIGO_TASK_PRIORITY_TIME` tasks and run ahead of a queued change handler, so a power or USB request copied while the finalizer came due was overwritten and republished (still BUSY) with the old values; the handler then read the overwritten values, sent the device its current state, read it back as matching and reported OK. The request was never applied. The window is narrow: only a change queued while the reboot completes successfully.
- **Fix:** `prodigy_publish_ports()` leaves the values of a BUSY AUX_POWER_OUTLET or AUX_USB_PORT alone and does not publish it. Both handlers send the request read item by item with `indigo_get_switch_target()`, apply it with `indigo_apply_switch_targets()` when the device accepted and reports it, and otherwise show the ports the device last reported with ALERT (the command order `X`/`Y` or `U`/`J`, then `D`, is unchanged).
- **Regression tests:** `power_request_survives_reboot` and `usb_request_survives_reboot` start a reboot, hold the shared device queue with a gate handler, send the request, let `reboot_finalizer` come due behind the gate (the simulator answers again 0.7 s after `Q`, the finalizer runs 1 s after it) and check that the first result after the request is the handler's OK with the requested values, published after the reboot's OK, that only the copy of the request published BUSY, and that `X:1` / `U:1` was sent exactly once. Against 3.0.0.7 both failed (first result OK with both items off, two BUSY publications, no `X:1` / `U:1` sent); both pass with 3.0.0.8, also in the ASan build.
- **Verification (Linux x64):** `TZ=Europe/Bratislava python3 tools/run_driver_test.py focuser_prodigy` 57/57 OK. Regeneration reproduces the checked-in output.
- **Observed, not changed:** the powerbox `on_connect` block publishes both port properties through `prodigy_publish_ports()` before the generated connection handler defines them, so the test client logs "updated without being defined" on every powerbox connect; this is unrelated to the target change and left as is. Fixed in 3.0.0.9 as TGT-D09, see below.

```sh
cd indigo_test && PRODIGY_TEST_FILTER=request_survives_reboot ./build/integration/test_focuser_prodigy_simulator
```

Final test summary: 57 simulated tests run, 57 passed; 0 hardware tests run, 0 passed.

## Port properties published before their definition (TGT-D09, 3.0.0.9, 2026-09-27)

Finding TGT-D09 in `indigo_drivers/REVIEW_SWITCH_TARGETS.md`, reproduced before the fix.

- **Defect (reproduced):** the powerbox `on_connect` block called `prodigy_publish_ports()`, which published AUX_POWER_OUTLET and AUX_USB_PORT with `indigo_update_property()` before the generated connection handler defines them right after the block. Every powerbox connect therefore sent clients one update of each undefined property; the definition that followed carried the same values.
- **Fix:** `on_connect` writes the ports read with `D` into the four items and no longer publishes them; the generated definition publishes them. `reboot_finalizer` still uses `prodigy_publish_ports()`, where both properties are defined. The protocol sequence of the connect (`#`, `D`) is unchanged.
- **Regression test:** `ports_defined_at_connect` connects the powerbox while the simulator answers the connect's `D` with `D:0:1:1:0`, disconnects, and connects again with `D:1:0:0:1`. After each connect the harness must have seen no update of an undefined property and no update of either port property, and the defined items must show the reported ports. Against 3.0.0.8 it failed 3/3 (two updates without definition and one update of each port property on the first connect); with 3.0.0.9 it passed 5/5, and in the ASan build.
- **Verification (Linux x64):** `TZ=Europe/Bratislava python3 tools/run_driver_test.py focuser_prodigy` 58/58 OK (`MIGRATION_STATUS.md` 57 / 0 -> 58 / 0). Regeneration reproduces the checked-in output.
- **Observed, not changed:** the recorded run still logs "updated without being defined" for focuser and powerbox `CONNECTION`, `INFO`, `FOCUSER_POSITION` and `FOCUSER_STEPS`. These come from the test harness: `driver_stop()` and the cases that switch between the two devices reset the single-device property cache (`reset_simulator_context()`), so later updates of properties that are still defined are counted. They are not driver updates of undefined properties.

```sh
cd indigo_test && PRODIGY_TEST_FILTER=ports_defined_at_connect ./build/integration/test_focuser_prodigy_simulator
```

Final test summary: 58 simulated tests run, 58 passed; 0 hardware tests run, 0 passed.

## Focuser testing rules alignment (3.0.0.10, 2026-10-05)

The suite was checked against the "Focuser Drivers" chapter of `indigo_test/DRIVER_TESTING_RULES.md` (commit fa5f64839). Most rules were already covered (connect refusal on every field, firmware variants, shared lifetime and sibling survival, additional instances, motion and limits, park, overlap refusal, queued abort, stop/start/stall failures, malformed, partial, overlong and silent polls, temperature, powerbox, reboot, disconnect during motion and park, transport loss). The simulator gained the one-shot actions `external=<target>` (hand-controller move) and `slow` (reply held 0.3 s, inside the driver timeout); the runner now plans its forked cases.

### Defects found and fixed

- `PRDG-1` An aborted move ended `FOCUSER_POSITION` and `FOCUSER_STEPS` OK. User decision: it ends ALERT at the stopped position with value equal to target, and later idle polls keep the ALERT (`failed_move`). Test: `abort`, `stop_failure` (two fresh polls read the same position and keep the ALERT).
- `PRDG-2` An abort with nothing moving sent `H` and republished the motion properties. It is answered OK without a command now (as is a request with the item OFF). Test: `abort`.
- `PRDG-3` An idle poll whose `I` reply was outstanding when a move request was accepted published the request OK before its handler ran. The poll re-checks after its reads and leaves a request the bus accepted alone; an external move it already follows still completes. Test: `request_survives_poll`.
- `PRDG-4` A failed sync set `uncertain`, so the next sync (and every move) was refused until an abort, and the requested value stayed as target. A failed sync now keeps the real position as value and target, stays ALERT through idle polls, and the next sync is accepted; a confirmed sync clears an uncertain stop. Test: `sync_failure`.
- `PRDG-5` A failed or aborted move's ALERT was turned OK by the next idle poll. Only an ALERT from a failed poll is cleared now. Test: `abort`, `sync_failure`.
- `PRDG-6` A limits change excluding the focuser was accepted, and a refused change left its target behind, so the next change was validated against it. Such changes are refused and the targets restored. Test: `limits`.
- `PRDG-7` A backlash change during motion sent `C`. It is refused without a command now, and `active` joins every motion `reject_change` so a refused property's ALERT cannot open the guard. Test: `rejected_change`.
- `PRDG-8` `X_AUX_REBOOT` reset the whole controller while the focuser sibling was connected (only a running move refused it). The reboot is refused while the shared connection count is above one. Test: `reboot_refused_with_focuser`.

Against the pre-fix 3.0.0.9 driver (built from a copy of its sources) 7 of the 61 scenarios fail: `rejected_change`, `abort`, `stop_failure`, `sync_failure`, `request_survives_poll`, `reboot_refused_with_focuser`, `limits`. `external_motion` documents behaviour the driver already had.

### Rules not applicable or deliberately kept

- Relative-only profile, temperature sentinel, compensation and mode: the controller is absolute, documents no "no sensor" value and has no compensation; reverse motion is fixed and not defined.
- Disconnect during motion sends `H` and then reads `P`/`I` before the port closes, to decide whether the stop is confirmed; nothing is sent after the close (`disconnect_motion`, `disconnect_park`).
- A refused speed change during motion is not required: speed is not a motion-geometry setting.

```sh
cd indigo_test && PRODIGY_TEST_FILTER=request_survives_poll ./build/integration/test_focuser_prodigy_simulator
```

The first two recorded runs failed `abort` (60/61): under the recorded run's timing the abort overtook the queued start, so the focuser stopped at its origin, a legitimate ALERT outcome the case did not expect. The case now waits for the move to make progress before it aborts, so it always lands mid-move as the rules require; the third recorded run passed.

Final test summary: 61 simulated tests in the recorded run, 61 passed (see README `## Testing`); 0 hardware tests run, 0 passed.
