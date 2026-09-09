# Refactoring plan for INDIGO 3.0 MGBox driver

Goal: migrate `aux_mgbox` to `indigo_generator`, preserving the combined Weather/Powerbox AUX and GPS logical devices, supported model capabilities and serial/TCP protocol. Replace handwritten lifecycle/property scaffolding with generated code and fix the existing reader/parser failures under simulator-backed tests.

Status (2026-09-09): Steps 1–8 completed for the software migration. The authoritative source is `indigo_aux_mgbox.driver`, version 10 (`0x0300000A`, baseline `0x03000004`). Final universal macOS builds, all 32 native arm64 simulator scenarios and nine selected ASan scenarios pass. Generated C/header/main are reproducible byte-for-byte. No generator changes were required. Hardware, TCP bridge and other-platform execution remain unverified. Intermediate failures below are retained as historical evidence.

## References and scope

- Repository `AGENTS.md`; `indigo_test/AGENTS.md` and `indigo_test/DRIVER_TESTING_RULES.md`, including common and GPS requirements.
- `README.md`, `TESTING.md`, `indigo_docs/DEVELOPMENT.md`, `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `indigo_docs/MAKEFILES.md` and `indigo_docs/SERIAL_DEVICE_SIMULATORS.md`.
- This folder's `README.md`, `indigo_aux_mgbox.c` and public header: current behavior. `indigo_docs/PROPERTIES.md`, section `aux_mgbox`: property inventory/source mapping.
- `indigo_drivers/aux_upb/indigo_aux_upb.driver`: generated AUX plus secondary-device structure. `indigo_drivers/mount_pmc8/REFACTOR.md`: shared-transport migration sequence. Inspect current generator output for AUX/GPS ownership before choosing the final layout.
- `doc/MGPBox Manual English 1.1.pdf`, pages 15–20, is the command reference identified by the existing simulator and `indigo_test/AUX_PROTOCOL_TESTS.md`. Read the actual manual before extending protocol behavior; current simulator reply spelling is partly based on the driver sample, not an independently documented reply contract.
- `aux_mgbox_simulator/aux_mgbox_simulator.c`, `indigo_test/integration/test_aux_mgbox_simulator.c`, `aux_test_isolation.h` and `indigo_test/AUX_PROTOCOL_TESTS.md`: existing PTY tests and their limitations.
- Baseline risks are recorded in `indigo_drivers/REVIEW.md` (`DRV-088`, `DRV-089`), now Closed (fixed) using the scoped migration evidence. This work does not advance folder review baselines.

## Baseline and public contract

- Baseline commit: `ddd689d88c87e9f2e37ad9175693cdece0a53917`.
- Initial workspace already contained modifications to `indigo.xcodeproj/project.pbxproj`, including MGBox simulator group cleanup and guider references. Preserve those edits when adding migration files later.
- Handwritten version: `0x03000004`. Entry point: `indigo_aux_mgbox`; label: `Astromi.ch MGBox`. The source's `DRIVER_NAME` is misspelled `idnigo_aux_mgbox`; generated metadata should use the correct name.
- Startup creates `MGBox Weather` (AUX weather/GPIO interface) and `MGBox GPS` (GPS interface), with GPS's `master_device` pointing at Weather. Both share one private-data allocation and transport. No hot-plug.
- README lists MBox, MGBox v1/v2, MGPBox and PBox. Source allows GPS when the identified name contains `G`, and pulse operation when it contains `P`. The README's GPS exclusion text and this predicate do not fully agree for MGBox; resolve against the manual/model evidence before turning that statement into simulator expectations.
- Serial baud defaults to `38400`; `mgbox://host:port` uses TCP with default port `9999`. Both logical devices currently expose port, port-list and baud properties, and the first connection opens using that logical device's settings. Generated master-port ownership is therefore a compatibility decision to document and test, not an invisible implementation detail.
- Both INFO properties have six items. GPS exposes advanced selection, three coordinate items and one UTC item. Weather exposes additional instances on the base instance, but global device/timer/parser storage prevents assuming instance isolation currently works.

### Property inventory

Inventory is based on all property/item initializers and count/hidden mutations in `indigo_aux_mgbox.c` (`gps_attach`, `aux_init_properties`, `aux_attach`), plus change/configuration handling. Preserve labels, item order, initial states, ranges and formats in the `.driver` declarations.

The AUX device's existing public name is `MGBox Weather`, but it combines two functional parts: Weather and Powerbox/PBox. `AUX_OUTLET_NAMES`, `AUX_GPIO_OUTLETS` and `AUX_OUTLET_PULSE_LENGTHS` belong to Powerbox, not Weather. Keep this distinction in the generated source organization and simulator coverage; the functional classification does not itself rename or split the public logical device.

| Functional part/property | Baseline schema and behavior |
| --- | --- |
| Powerbox `AUX_OUTLET_NAMES` | Always defined, RW text, `GPIO_OUTLET_NAME_1` defaults to `Pulse switch`; saved by CONFIG. Changes redefine outlet and pulse-length labels. |
| Weather `AUX_DEW_THRESHOLD` | Always defined, RW number, `AT_SENSOR_1`, 0–9, step 0, default 2 °C; saved by CONFIG. |
| Powerbox `AUX_GPIO_OUTLETS` | Connected-only RW any-of-many switch, `OUTLET_1`, default false. Pulse request rejected with ALERT on models without `P`. |
| Powerbox `AUX_OUTLET_PULSE_LENGTHS` | Connected-only RW number, `OUTLET_1`, 1–10000 ms, step 100, default 1000; local setting, not explicitly saved. |
| Weather `AUX_WEATHER` | Connected-only RO number, initial BUSY; temperature −200–80, dewpoint −200–80 (step 1), humidity 0–100, pressure 0–10000. Formats `%.1f` except pressure `%.2f`. Incoming pressure Pa is divided by 100 for hPa. |
| Weather `AUX_DEW_WARNING` | Connected-only light, `AT_SENSOR_1`; property initially BUSY, item IDLE. ALERT when temperature minus threshold is at or below dewpoint. |
| Weather `X_WEATHER_CALIBRATION` | Connected-only RW number, temperature −200–200, humidity −99–99, pressure −999–999, step/default 0; wire values scaled by ten. Existing pressure label says Pa; preserve unless protocol evidence justifies a separate correction. |
| Weather `X_SEND_WEATHER_DATA_TO_MOUNT` | Connected-only RW any-of-many switch `ENABLED`, default false; sends `:mm,0*`/`:mm,1*`, confirmed by CAL data. |
| Weather `X_REBOOT_DEVICE` | Connected-only RW any-of-many switch `REBOOT`, default false; sends `:reboot*` and clears the switch. |
| GPS `X_SEND_GPS_DATA_TO_MOUNT` | Connected-only RW any-of-many switch `ENABLED`, default false; sends `:mg,0*`/`:mg,1*`, confirmed by CAL data. |
| GPS `X_REBOOT_GPS` | Connected-only RW any-of-many switch `REBOOT`, default false; sends `:rebootgps*` and clears the switch. |
| GPS inherited properties | `GEOGRAPHIC_COORDINATES` count 3, `UTC_TIME` count 1, `GPS_ADVANCED` visible. RMC/GGA provide position/time/elevation, GSA fix/DOP and GSV satellite count. |

The `AUX_*` names above have standard definitions in `indigo_names.h`, including `AUX_OUTLET_PULSE_LENGTHS`; retain their standard names. All five driver-specific properties already start with `X_`. Do not rename standard properties merely because the inventory groups them as custom allocations.

## Open review findings required for migration acceptance

Re-read `indigo_drivers/REVIEW.md` on 2026-09-09. Both MGBox findings remain High/Open; the baseline simulator run reproduced them. Fixes are required parts of this migration, not deferred cleanup.

| Finding and source | Implementation step | Required regression evidence before closing |
| --- | --- | --- |
| `DRV-088`, `indigo_aux_mgbox.c:204` (GPS), also weather parsing at line 345: missing fields in checksummed truncated sentences cause NULL dereferences. | Step 4: bounded parsing and reply validation. | `short_gps` and `short_weather` no longer crash, reject malformed frames and recover on valid input; targeted driver-instrumented ASan passes. |
| `DRV-089`, `indigo_aux_mgbox.c:195`, open at line 431 and close at lines 473–475: pointer handles treated as integer descriptors break failed open and last disconnect. | Step 3: transport/reader lifetime; revalidate after generated ownership in steps 6–7. | Invalid port reaches disconnected ALERT; Weather and GPS disconnect/shutdown complete; shared connection orders, failed acquisition rollback and reconnect pass with bounded reader teardown. |

Update the existing finding statuses only after the corresponding fix and regression evidence, preserving their stable IDs. Do not advance the folder's last-reviewed commit for this scoped migration work.

## Migration constraints established from the source

- `mgbox_open()` and the reader compare `indigo_uni_handle *` with `>= 0`. NULL enters the success/read path, and last close cannot reliably stop the reader (`DRV-089`). Use pointer validity and explicit ownership, not descriptor comparisons.
- `parse()` uses static token storage, has no token-capacity guard and does not validate fields before GPS/weather conversion (`DRV-088`). Move parsing into bounded, instance-safe helpers and validate transport replies before publishing data.
- The reader ignores its callback device and uses global GPS/Weather pointers. It runs an indefinite loop from a timer and updates both logical devices. Replace this with bounded work associated with the physical instance; never move the indefinite loop unchanged onto a serialized queue.
- Existing open increments `count_open` before acquisition and starts the reader before identification. Identification depends on that concurrently running reader. A queued replacement must parse identification within the bounded open operation or another explicit acquisition design; waiting on work queued behind the current handler would deadlock.
- The unchanged generator must own shared connection references through `PRIVATE_DATA->count`. Remove the competing `count_open` during migration. `mgbox_open()` must acquire everything or unwind everything; `mgbox_close()` must run only after successful acquisition, including rollback when GPS capability initialization fails.
- Preserve the 0.5-second command-spacing requirement until protocol/hardware evidence permits changing it. Check write results. Pulse duration and reboot recovery waits should use operation finalizers rather than sleeping through the full operation.
- CAL replies are unsolicited/readback completion for calibration and forwarding. A first intermediate CAL response must not falsely complete a multi-command calibration; confirmation and timeout ownership need explicit handling.
- Shared properties are accessed via the correct logical-device context. A slave handler already executes on the master's queue with the original slave argument; custom queue redirection is unnecessary.
- Use generator defaults, no `MAX_DEVICES` override. No generator implementation/DSL/lifecycle changes are authorized by this migration: if a concrete limitation appears, prepare its exact proposed diff and impact for separate approval.

## Atomic implementation sequence

Each numbered step is a separately reviewable patch with its own build/test result. Commit creation is not required. Keep each step's tests and results here, coverage/deferred work in `indigo_test/CHANGES.md`, and risk dispositions in the existing review file. Once `.driver` exists, it is authoritative and every subsequent change regenerates C/header/main.

1. **Establish the baseline (completed).** Inventory source, properties, resource ownership, existing changes and simulator capabilities. Rebuild the unchanged driver and run all five existing scenarios. Record failures as failures, not expected passes. Commands/results are below.

   Result (2026-09-09, confirmed on the explicit Step 1 request):
   - HEAD remains `ddd689d88c87e9f2e37ad9175693cdece0a53917`. Host is Darwin arm64; compiler is Apple clang 21.0.0 (`clang-2100.1.1.101`), targeting `arm64-apple-darwin25.6.0`. Build configuration emits both x86_64 and arm64.
   - The forced production build, simulator/test build and five-scenario execution were already performed during preparation in this same task. `git diff --exit-code HEAD --` confirmed that production C/header/main, simulator, integration test, isolation helper and test Makefile still match that baseline. Reuse those actual results below; no duplicate execution or new passing result is claimed.
   - Rechecked all property/item initializers, count/hidden assignments and CONFIG save calls. There are 11 driver-allocated properties with 16 items: nine properties on the combined AUX device and two on GPS. Three AUX properties form the Powerbox part; their ownership is explicitly distinguished from Weather in the inventory. Only outlet names and dew threshold are explicitly saved by this driver.
   - Resource ownership is recorded in the table below. Existing `DRV-088` and `DRV-089` remain Open, with required fixes and regression evidence mapped above. Step 1 records their failures; it does not fix or waive them.
   - `make -C indigo_test test-clean` completed after the baseline run; the test build directory is confirmed absent. The suite parent completed and executes simulator cleanup after each child, including crash cases. Independent process-list verification was unavailable (`pgrep`: sysmond service not found); do not claim a separate process census passed.
   - Current documentation changes are this plan and `indigo_test/CHANGES.md`; the pre-existing Xcode project edits remain. Production behavior and version remain unchanged at `0x03000004`. Next step is Step 2, Powerbox/PBox simulator coverage.

   | Resource / baseline source | Acquisition and current release contract | Migration obligation |
   | --- | --- | --- |
   | Shared private data and logical devices, `indigo_aux_mgbox.c:1030` | INIT allocates shared data, then AUX and GPS; GPS points to AUX as master. SHUTDOWN verifies both disconnected, detaches/frees AUX then GPS, then shared data. | Generated ownership must keep shared data alive for both devices and pending operations. |
   | Transport and `count_open`, `indigo_aux_mgbox.c:419` | First open increments before acquisition, opens serial/TCP and probes identity. Failed identification closes and cancels the reader before decrement; final close decrements to zero, closes, then waits for the reader. | Transactional acquisition, reader quiescence before close and generated reference ownership; fix `DRV-089`. |
   | Global reader timer, `indigo_aux_mgbox.c:189` | Started against global GPS even for AUX-only use; reads and updates both global devices. Cancellation occurs on failed identification and final close. | Per-instance bounded reads, no detached-device updates or indefinite queue occupation. |
   | Serial/reset mutexes, `indigo_aux_mgbox.c:812` | AUX attach initializes both; commands/open/close use serial mutex and reader/reboots use reset mutex. No matching mutex destruction in detach. | Eliminate obsolete locks with queue serialization, or explicitly balance lifetime for any lock retained. |
   | Custom property allocations, `indigo_aux_mgbox.c:536`, `:721` | GPS owns two allocations, AUX nine. Their detach callbacks release them; AUX also deletes the two always-defined properties. | Let the generator allocate/define/delete/release declarations, retaining visibility and persistence. |
   | Pulse/reboot/calibration callbacks, `indigo_aux_mgbox.c:609`, `:867` | Change handlers schedule timer work without retained timer handles; forwarding also writes directly from the bus callback. | Queue work and named finalizers with explicit disconnect/pending-operation ownership. |

2. **Make the simulator cover the PBox pulse function (completed).** The user specifically identified this gap. Baseline simulation only accepted `:pulse,<ms>*`, stored `pulse_until`, and cleared that variable later; no test observed the pulse. Add model profiles with documented capability provenance (including standalone PBox) and observable command/timing traces. Reuse the ready-file contract and existing trace facilities where practical; any additional observation channel is test infrastructure, not invented device protocol. Add public-bus tests for pulse length, exactly one pulse command, switch reset, model without pulse, and GPS rejection on PBox. Keep full weather/GPS profiles. Validate emitted traces independently even while known driver teardown failures remain. Do not invent power rails, voltage/current sensors or heaters absent from this driver's interface.

   Result (2026-09-09):
   - Read the bundled MGPBox manual, including application-interface pages 15–20. Page 19 explicitly describes `:pulse,1500*` as a 1.5-second relay pulse; page 20 documents `:devicetype*`. The manual does not specify the identity reply spelling or standalone PBox/MBox firmware. Keep those profiles labelled as capability fixtures based on the driver/README rather than claiming independent manufacturer validation. MGBox v1/v2 capability ambiguity remains outside this step.
   - Extended the existing host-side simulator with `pbox` (Powerbox only) and `mbox` (Weather only); `normal` remains MGPBox with Weather, GPS and Powerbox. The two malformed-message profiles are preserved. PBox emits no fabricated weather/GPS stream; MBox rejects pulse commands. Powerbox is still part of the existing combined AUX logical device, not a third device.
   - Added optional `INDIGO_MGBOX_SIMULATOR_EVENTS` journal with monotonic timestamps and RX/TX/PULSE_ON/PULSE_OFF/REJECT events, flushed for observation across processes. It adds no wire commands/replies and changes no shared simulator helper. The parent owns the private temporary journal, the simulator truncates it for each scenario, and the parent unlinks it after the suite. Pulse completion uses the existing 0.5-second simulator tick; timing assertions accept 1.5–2.2 seconds for the observed 1500 ms pulse. This is simulator scheduling, not physical relay accuracy.
   - Added three independent PTY simulator checks and four public-bus driver scenarios. The pulse tests also verify `AUX_OUTLET_NAMES.GPIO_OUTLET_NAME_1` updates both Powerbox labels and that changing the length alone sends no pulse. Corrected the initial inventory's mistaken outlet-name item spelling; no public property changed.
   - Added the production archive prerequisite to the narrow test target, ensuring future driver changes relink the executable. Build of simulator and suite passed for macOS x86_64/arm64 without compiler diagnostics. Full native arm64 run: 12 scenarios, 3 pass and 9 fail, exit 1; detailed results below. Known failures remain ordinary failures, not expected passes.
   - Tightened pulse-reset observation to reject reset before the 1500 ms duration (50 ms observation tolerance). Rebuilt and reran `AUX_TEST_FILTER=powerbox_ ./build/integration/test_aux_mgbox_simulator`: all three control sections passed again, including that reset timing check, and all three scenarios still failed only at disconnect/shutdown. The full-suite result above precedes this targeted assertion refinement; unchanged scenarios were not rerun unnecessarily.
   - Production C/header/main still match HEAD; DRIVER_VERSION remains `0x03000004` because this step changes only the host simulator, tests and documentation. The host simulator is a standalone test program, not an INDIGO simulator driver with a DRIVER_VERSION. No generator changes, driver property additions/removals or framework changes were made.
   - Updated `indigo_test/AUX_PROTOCOL_TESTS.md` and `indigo_test/CHANGES.md` with profiles, journal contract, evidence and remaining coverage. `DRV-088`/`DRV-089` remain Open. No ASan, hardware, TCP, Linux, Windows or x86_64 execution is claimed for this step.
   - Final `git diff --check` passed. Both test parent processes completed, with journal removal and simulator reap in their cleanup paths. `make -C indigo_test test-clean` passed and removed the test build artifacts. Pre-existing Xcode edits were preserved.

   | Scenario | Observed result |
   | --- | --- |
   | `simulator_mgpbox` | PASS: identity, weather/GPS streams, exact pulse command and timed ON/OFF events. |
   | `simulator_pbox` | PASS: PBox identity, exact pulse command and timed ON/OFF, no weather/GPS stream. |
   | `simulator_mbox` | PASS: MBox identity/weather, pulse rejection, no GPS or pulse activation. |
   | `powerbox_mgpbox`, `powerbox_pbox` | Label, pulse command/duration and switch-reset assertions pass; final disconnect/shutdown fails (`DRV-089`). |
   | `powerbox_unavailable` | MBox publishes outlet ALERT/false, with no pulse command or activation; disconnect/shutdown fails (`DRV-089`). |
   | `pbox_rejects_gps` | Fails to reach disconnected ALERT within the bounded wait. Source rejects the model through `mgbox_close()`, whose reader cleanup hangs before the rejection is published (`DRV-089`); no successful GPS rejection is claimed. |
   | `normal`, `gps_readings` | Original data assertions pass; disconnect/shutdown still fails (`DRV-089`). |
   | `short_weather`, `short_gps` | Original SIGSEGV failures remain (`DRV-088`), reserved for Step 4. |
   | `invalid_port` | Original disconnected-ALERT/teardown failure remains (`DRV-089`), reserved for Step 3. |

3. **Fix transport and reader lifetime in the handwritten driver.** Correct pointer checks, transactional acquisition/failed identification, per-instance reader ownership and cancellation before handle destruction. Ensure first/last connection reference handling is balanced and a failed secondary connection leaves the first usable. Bump DRIVER_VERSION above `0x03000004`. Run normal Weather, GPS, invalid port, repeated connect/disconnect, silent identification and both shared connection orders. Parser-fault scenarios remain explicitly failing until step 4.

   Result (2026-09-09, Step 3):
   - Handwritten version advanced to `0x03000005`. Replaced pointer/descriptor comparisons, moved the shared reference increment after successful acquisition, and stopped/joined the per-instance reader before closing its handle. Added per-instance logical-device/timer references, atomic reader cancellation and balanced mutex destruction. The later queue migration will remove this temporary reader/lock architecture.
   - Production universal macOS build passed. The 12-scenario suite now passes 10 scenarios, including Powerbox teardown, GPS rejection and invalid port; only the two Step 4 malformed-frame crashes remain.
   - Added silent-identification and invalid-port-to-three-reconnect regression scenarios; both pass. Shared connection-order tests initially exposed a test-cache limitation: a duplicate connect produces no update to repopulate the reset observation context. Changed the observer to enumerate and inspect the already-connected property, rather than requiring a duplicate connection notification. Both corrected shared-order tests passed.
   - No property was added or removed. Known parsing faults remain open for Step 4; final shared ownership acceptance will be repeated after generation.

4. **Make protocol parsing safe and deterministic.** Bound tokens and sentence lengths; validate required fields, numeric conversions and checksum syntax; preserve supported framing semantics. Cover truncated RMC/GGA/GSA/GSV/XDR/CAL/LOG, malformed numeric replies, bad checksums and recovery with a subsequent valid frame. Cover hemisphere signs, UTC, fix loss/reacquisition and satellite/DOP updates where supported. Prevent invalid/no-fix input from becoming valid fresh position/time. Bump the driver version again. Adapt existing short-only fixtures so rejection/recovery assertions replace their current expectation of normal readings. Run all narrow scenarios and the driver-instrumented ASan target.

   Result (2026-09-09, Step 4):
   - Handwritten version advanced to `0x03000006`. Replaced static token storage and unchecked conversions with bounded per-instance framing, checksum syntax verification, field counts, finite numeric/domain checks, coordinate/time validation and no-fix handling. Partial input survives reads; overlong frames are discarded and framing resynchronizes at the next sentence.
   - Short-message fixtures now emit a malformed frame then valid data, checking recovery rather than waiting forever for a normal value from a permanently truncated stream. Added a fault burst covering RMC/GGA/GSA/GSV/XDR/CAL/LOG, non-finite numbers, bad checksum syntax and overlong input.
   - Production and test builds passed. All 18 scenarios passed on native macOS arm64. Both `short_` and both `parser_` scenarios passed with the production driver instrumented by AddressSanitizer; the linked framework is not instrumented. The ASan build exposed legacy sprintf deprecation warnings, which the subsequent source migration removes.

5. **Move operations into generator-compatible queues and helpers.** Use one shared master queue for bounded stream reads and commands. Eliminate global timer/device/token storage and obsolete mutexes only after proving there is no independent reader accessing the transport. Introduce pulse/reboot/calibration/forwarding finalizers with accurate BUSY, OK and ALERT updates and bounded completion. Let framework BUSY guards reject overlap; cancel only identified pending work on disconnect/reboot or a documented cross-property conflict. Test command spacing, pulse overlap, calibration readback ordering, forwarding both ways, both reboot controls, transport failure and disconnect during pending operations. Bump version for behavioral fixes.

   Result (2026-09-09, Step 5):
   - Replaced the temporary reader thread/mutex architecture with bounded reads and command work on the framework's shared master queue. Polling retains each logical device as the callback argument, so generated disconnect cancellation removes only that device's pending work and the connected peer continues polling. Framing, model, fix and operation state are per instance; the only shared reference counter is generated `PRIVATE_DATA->count`.
   - Added pulse, device/GPS reboot, calibration and weather/GPS forwarding finalizers. Retained the existing 0.5-second command spacing, but pulse/reboot/readback waits are delayed handlers rather than long sleeps. Calibration preserves requested targets until all three readback values match; forwarding confirms its own returned flag. Missing replies reach ALERT after 50 polling attempts. A CAL arriving after the framework's BUSY update but before the queued operation starts must not prematurely publish OK; the dedicated observer verifies this ordering.
   - Framework BUSY guards exclude duplicate pulse requests. Reboot checks cover actual cross-property conflicts. Disconnect cancels pending finalizers and clears their local states so an inactive logical device cannot block its peer; physical pulse cancellation is not claimed. Transport EOF stops polling and publishes ALERT, and controls addressed to a disconnected logical device do not send commands through its connected peer.
   - Steps 5 and 6 were integrated in the new `.driver` because queue ownership and generated connection lifetime must change together. There was no separate retained handwritten queue implementation. Generated version 8 passed all 18 then-existing scenarios; version 9 passed all 29 expanded scenarios. Final version 10 also corrects socket-write timeout units to `INDIGO_DELAY(2)` and disconnected-operation state handling. TCP execution remains untested.

6. **Migrate lifecycle and properties to `.driver`.** Write `indigo_aux_mgbox.driver` with `driver mgbox`, `serial;`, AUX master then GPS slave, existing names/interfaces and minimal private state. Use exact `mgbox_open(indigo_device *device)`/`mgbox_close(indigo_device *device)` helpers and generator-owned references. Choose and document port ownership using actual generated output; test GPS-first connection using the master's port when following the generated convention. Use `connection_result` for GPS capability rejection, without returning from connection hooks. Express local copy-only properties with empty `on_change { }`, persistence/visibility through attributes, and remove handwritten allocation/dispatch/attach/detach scaffolding. Preserve the license, extend copyright through 2026, and add the Codex refactoring notice after it. Set `version` higher than the latest handwritten version. Generate `.c`, `.h`, `_main.c` together. If reverse extraction is used, invoke `indigo_generator -c indigo_aux_mgbox.driver`, never with the existing `.c` as output. Build and run the complete narrow simulator suite immediately.

   Result (2026-09-09, Step 6):
   - Added authoritative `indigo_aux_mgbox.driver` with AUX master and GPS slave, original public names, interfaces, item order/ranges/formats, standard Powerbox properties and all five `X_` properties. Empty change blocks implement pulse-length and dew-threshold copies; generator attributes implement always-defined/persistent names and threshold, and inherited GPS visibility.
   - Used exact transactional `mgbox_open()` / `mgbox_close()` helpers. The unchanged generator owns reference increments after successful open, secondary initialization rollback, connection updates, property allocation/definition/deletion/release, dispatch, attach/detach and additional instances. There is no custom `MAX_DEVICES` override, old `count_open`, reader thread, mutex or global timer/parser/device storage.
   - Generated port ownership intentionally replaces the old first-connected-logical-device convention: configure port/baud on `MGBox Weather`, including GPS-only connections. GPS's own port controls remain inherited/hidden. Both GPS-first and AUX-first paths use the master's port in the tests. Generated headers retain the public entry point and remove the private device-name macros; tests use the public names.
   - Preserved the full original license in the DSL, extended copyright through 2026 and added the Codex notice. The unchanged generator emits its standard equivalent license and generated-source notice in C/header/main. Corrected the legacy misspelled DRIVER_NAME through generated metadata. Production and test universal builds passed; the immediately generated version passed all 18 scenarios before coverage expanded.
   - A reverse-extraction trial was isolated in a temporary directory and was not used as authoritative input because it misassigned some legacy property scaffolding. No existing C source was passed as the extraction output path.

7. **Verify generated ownership and instance isolation.** Inspect every generated handler, especially `_finalizer` references that suppress the generator's OK prologue/final update. Remove duplicate state updates and BUSY checks. Test Weather-only, GPS-only, both connection/disconnection orders, secondary failure with primary active, reconnect after failure, disconnect during a pulse/reboot/read, and fresh data after reconnect. Test two independent additional instances against separate PTYs; no shared parser, port, model or pulse state may leak. Use the normal single-device cache with separate lifecycle cases and a scoped observer for simultaneous-device assertions rather than broad harness changes. Run narrow tests and targeted ASan after this step.

   Result (2026-09-09, Step 7):
   - Inspected generated connect/disconnect rollback and every change handler. Copy-only changes and outlet-name completion use generator epilogues. Operations with `_finalizer` rely on the framework's BUSY/start publication and own their explicit failure/delayed completion updates; no duplicate BUSY guards or generator completion updates were added. CONFIG saves exactly outlet names and dew threshold. All eleven allocated driver properties and inherited INFO/GPS count/visibility changes match the documented inventory.
   - Final version `0x0300000A` passed all 32 ordinary scenarios on native macOS arm64, including clean disconnect and shutdown. Coverage groups: 3 independent simulator model checks; 4 Powerbox/capability cases; 6 settings/timeout/overlap/reboot-disconnect cases; pulse-disconnect and transport-loss cases; 2 GPS fix/hemisphere/UTC cases; 5 shared/failure/reconnect/instance cases; silent identification; normal Weather/GPS; 6 malformed/split-frame cases; invalid port. Exact registered names are in the integration suite.
   - Nine final driver-instrumented ASan cases passed: both `short_`, both `parser_`, both `split_`, `pulse_disconnect`, `reboot_disconnect`, and `instances`. These include checksum syntax/value failures, excess fields, long frames, truncated message families and recovery; no sanitizer findings occurred. The existing linked framework is not instrumented.
   - Pulse teardown completes in under three seconds while a ten-second simulated pulse is active, followed by successful reconnect. Reboot disconnect occurs while the reboot property is BUSY, then reconnect restores fresh weather. Shared cases retain the peer's data after one logical disconnect; failed GPS capability initialization leaves the primary alive. Additional-instance data remain distinct on two independent PTYs after disconnecting either AUX owner.

8. **Synchronize integration and document acceptance.** Add `.driver` and this plan to the relevant Xcode group while preserving initial project edits; inspect Windows project/filter conventions for source visibility. Update `PROPERTIES.md` to point at `.driver`, documenting any intentional visibility/behavior changes in the same patch. Update README for confirmed model/port behavior; update test coverage notes and only verified risk dispositions. Regenerate again and compare C/header/main byte-for-byte; inspect the complete generated diff and run `git diff --check`. Rebuild production and force relinking of the narrow test against the final archive before running it. Clean test artifacts and confirm no simulator/test processes remain.

   Result (2026-09-09, Step 8):
   - Added the `.driver` to Xcode's MGBox group; its REFACTOR reference was already present in the user's initial project changes. Preserved all initial simulator/guider project edits. Added non-build `.driver`/REFACTOR entries to the Windows project and filters. Xcode plist lint and project XML parsing pass; no Windows compilation is claimed. Existing Windows line endings are preserved outside the added LF lines.
   - Updated `PROPERTIES.md`, driver README, `AUX_PROTOCOL_TESTS.md` and `indigo_test/CHANGES.md` for generated source ownership, master-port configuration, Powerbox classification, observable async completion, fixture provenance and remaining hardware coverage. Closed only `DRV-088`/`DRV-089` with verified regression evidence in the driver review file; no review baseline or unrelated finding was changed.
   - Final production C/header/main compiled and linked for x86_64/arm64 without diagnostics. The archive prerequisite relinked the final ordinary test; its 32-case run exited 0. The final ASan build and all nine selected cases exited 0. Regeneration after that run produced byte-identical C/header/main, verified with SHA-256 comparisons. Checked version increase, custom-name prefixes, all property/item/count/hidden declarations, absence of `MAX_DEVICES` override and obsolete reader/reference scaffolding, and reviewed generated lifecycle differences.
   - Final `git diff --check` passed. `make -C indigo_test test-clean` removed the test build directory. Every launched test parent returned and executed its simulator reap/private-journal removal paths, including the extra instance simulator; no server was launched. An independent process-list check was unavailable (`pgrep`: sysmond service not found), so no separate process census is claimed. Temporary migration source/extraction files were removed.
   - Software acceptance is complete. Physical relay pulse duration, real reboot/reset recovery, TCP bridges, MGBox v1/v2 and standalone firmware differences, Linux/Windows and x86_64 runtime remain explicitly outside the observed results.

## Validation commands and recorded results

Run from the repository root unless a working directory is shown:

```sh
make -C indigo_drivers/aux_mgbox -f ../../Makefile.drv -W indigo_aux_mgbox.c -W indigo_aux_mgbox_main.c
make -C indigo_test build/integration/test_aux_mgbox_simulator
```

Run the executable from `indigo_test` because the simulator path is relative:

```sh
./build/integration/test_aux_mgbox_simulator
```

Step 2 added the driver archive as a prerequisite, so the normal narrow build now relinks after driver changes. An explicit forced relink and the optional ASan build remain available:

```sh
make -C indigo_test -W integration/test_aux_mgbox_simulator.c build/integration/test_aux_mgbox_simulator
make -C indigo_test build/integration/test_aux_mgbox_simulator_asan
```

For a targeted parser check, run `AUX_TEST_FILTER=short_ ./build/integration/test_aux_mgbox_simulator_asan` from `indigo_test`. ASan instruments the driver/test, not the existing framework library. After each finished validation batch use `make -C indigo_test test-clean`.

Baseline on 2026-09-09:

- Forced production compilation and archive/dylib/executable linking passed for macOS x86_64 and arm64, without compiler diagnostics. Test and PTY simulator builds also passed for both architectures.
- Native macOS arm64 execution: Weather data/calibration and GPS coordinate assertions passed, but both scenarios failed disconnect/shutdown. `short_weather` and `short_gps` terminated with SIGSEGV (signal 11). `invalid_port` failed to reach disconnected ALERT within the bounded wait and also failed teardown. Overall: all five scenarios failed; executable exit status 1.
- These reproduce the already recorded `DRV-088`/`DRV-089`; they are not caused by migration. No hardware, TCP, Linux, Windows or x86_64 execution is claimed. ASan was not rerun in this preparation step.

## Completion criteria

- Authoritative `.driver` and reproducible generated C/header/main; production and narrow test builds pass, with all fixed-driver versions increased.
- All existing failures are resolved and simulator tests cover both logical devices, standalone PBox/pulse behavior, shared ownership, failure rollback, malformed data and asynchronous operation teardown.
- No manual generator boilerplate, duplicated shared reference counter, unbounded reader on the queue, global per-instance state, leaked handle or callback after detach.
- Standard names and all `X_` properties, persistence and intentional compatibility changes are documented. Simulator command acceptance alone is never reported as tested control behavior.
- Physical MGPBox/PBox timing, serial reset behavior, TCP bridge operation and other platforms remain explicitly unverified until exercised. Do not mark hardware acceptance complete from PTY results.


## Follow-up: compiler conversion warnings (2026-09-09)

The normal Makefile build did not enable Xcode's `-Wshorten-64-to-32` check. Reproduced its warning in checksum parsing: `strtoul()` returned `unsigned long` into `unsigned int`. Changed the checksum accumulator/expected type to `unsigned long` in the authoritative `.driver` and regenerated all outputs. Also made the already bounded GPS time/date/fix conversions and short command-length conversion explicit, resolving the additional diagnostics exposed by `-Wconversion`. These changes preserve behavior; version remains `0x0300000A`.

Validation: universal x86_64/arm64 production build passed. Both architectures passed syntax checking with `-Wall -Wextra -Wno-unused-parameter -Wshorten-64-to-32 -Wcomma -Wconversion -Werror`. Native arm64 `parser_weather`, `parser_gps` and `gps_southern` simulator cases passed, covering checksum rejection/recovery, GPS parsing and UTC year rollover. This targeted follow-up did not rerun the earlier full suite or ASan. Regenerated outputs remain byte-for-byte reproducible, `git diff --check` passed, and `make -C indigo_test test-clean` removed the test artifacts.


## Follow-up: redundant attach state initialization (2026-09-09)

Removed the property-level `on_attach` blocks for `AUX_DEW_WARNING` and `AUX_WEATHER` from the authoritative `.driver` and regenerated C/header/main. Both properties are connection-dependent; `aux.on_connect` still sets them to BUSY before publication, and valid weather input supplies their subsequent state. The attach assignments duplicated this initialization without affecting the public behavior. Version remains `0x0300000A`.

Validation: universal x86_64/arm64 production/test builds passed, as did strict syntax checks for both architectures with `-Wconversion`, `-Wshorten-64-to-32` and `-Werror`. The native arm64 `normal` simulator scenario passed data/control assertions and disconnect/shutdown. Confirmed the two generated attach blocks are absent and connection initialization remains. `git diff --check` passed; `make -C indigo_test test-clean` removed test artifacts. No full-suite or ASan rerun was needed for this redundant-initialization removal.


## Follow-up: migration status tracking (2026-09-09)

Updated the `aux_mgbox` row in root `MIGRATION_STATUS.md` to generated code, async queues, simulator retesting and available automated tests, with the recorded 32-scenario/nine-ASan evidence and hardware/TCP/Windows-runtime limitations. Preserved the existing Windows support designation; no new Windows validation is claimed. Added a repository-wide rule in `AGENTS.md` requiring this status update after every completed driver migration, consistent with its `REFACTOR.md`. This documentation-only follow-up was checked with `git diff --check`; no build or simulator rerun was needed.
