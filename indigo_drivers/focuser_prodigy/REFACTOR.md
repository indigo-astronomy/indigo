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
