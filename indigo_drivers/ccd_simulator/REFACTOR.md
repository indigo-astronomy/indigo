# CCD Simulator Refactoring

## Scope and current-state audit

The refactoring scope is `indigo_ccd_simulator`: migrate the hand-written multi-device simulator to `indigo_generator`, preserve its observable behaviour, move prolonged work to handler queues, complete the applicable automated coverage, and keep the generated sources and project metadata synchronized.

The current driver is a virtual, platform-independent simulator with one shared `simulator_private_data` allocation and nine logical devices:

- five CCD devices: Imager, Guider Camera, Bahtinov, DSLR, and File;
- one wheel and one focuser attached to the Imager master device;
- one guider and one AO device attached to the same master device.

The shared state owns generated image buffers, file-image buffers, star-field and hot-pixel data, simulated temperature, wheel/focuser/guider/AO state, timer handles, and pointers identifying the five CCD variants. The checked-in implementation is hand-written in `indigo_ccd_simulator.c`; there is no `.driver` input. The public header and standalone `_main.c` are also hand-written. Static image data remains in `indigo_ccd_simulator_data.c` and `indigo_ccd_simulator_data.h`.

Public behaviour includes ordinary and streaming CCD exposures; FITS/XISF/JPEG/RAW/native formats; frame, binning, gain, offset and gamma controls; imager cooling and temperature polling; generated star fields and guider-image setup; Bahtinov focus images; DSLR program/exposure metadata; loading raw images from a file; five-position wheel motion; absolute/relative focuser motion, sync, abort, backlash and temperature simulation; four-direction guider pulses and guide rate; and four-direction AO offsets and reset.

The simulator has no manufacturer protocol or vendor SDK. Repository documentation is the driver `README.md`, `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `indigo_docs/MAKEFILES.md`, the CCD/guider/AO/wheel/focuser matrices in `indigo_test/DRIVER_TESTING_RULES.md`, and the implementation/tests themselves. No `README.md` change is authorized or planned.

### Defects and risks found before implementation

- The existing streaming callback blocks in a loop and sleeps for each exposure. It does not use monotonic deadlines and can occupy its worker for an unbounded stream; migration must use a start handler plus bounded `streaming_finalizer` iterations and preserve exact fractional exposure timing.
- Ordinary exposure completion uses legacy timers and does not call `indigo_ccd_exposure_setup()`. Migration must use the shared CCD countdown and a named finalizer.
- Connection, wheel, focuser and guiding work uses legacy timers rather than generated handler queues. Disconnect/abort/replacement cancellation and stale completion need explicit queue-safe ownership.
- Image creation is protected by `pthread_mutex_t image_mutex` because several CCD logical devices share buffers/state and can currently execute concurrently. The migration must either retain a justified shared-state lock or redesign ownership/serialization; it must not assume separate logical-device queues serialize sibling devices.
- A single shared private-data allocation means shutdown order matters: children must be detached before the Imager master and the shared allocation must be freed exactly once. The generator in this worktree already contains the separately approved child-before-master virtual-device detach fix.
- File-camera connection has rollback-sensitive allocations and file-handle paths. Every failed open attempt must close the handle and release partially allocated buffers before reporting `CONNECTION/ALERT`.
- The current generator identifies generated functions, templates and static device variables only by the INDIGO device class. Repeating `ccd {}` therefore emits duplicate `ccd_attach`, `ccd_template` and `ccd` identifiers. The simulator requires five independently named CCD descriptions, so migration is blocked on an explicitly approved generator/DSL extension or an explicitly approved reduced-scope alternative.
- The current driver version is `0x03000019`; successful behavioural migration must increase the `.driver` version and regenerate it into the checked-in sources.

### Platform, build and packaging audit

The source uses portable INDIGO APIs except for the internal pthread image mutex; it declares platform-independent support. The existing macOS build produces a universal arm64/x86_64 driver and test binary. Linux and Windows execution environments are not available in this workspace. Windows packaging is driven by the repository driver build tooling rather than a per-driver Visual Studio source project; the final static audit must verify that generated sources introduce no platform-specific API and that the `.driver`/documentation are registered wherever this repository enumerates persistent driver files. Existing CCD sources and integration tests are present in the Xcode project; the new `.driver` and this record are not yet registered.

### Existing automated coverage and gaps

`indigo_test/integration/test_ccd_simulator.c` currently contains 17 hardware-free scenarios. It covers driver metadata and lifecycle; all nine logical-device compliance paths; short CCD exposures; imager countdown progress, abort and restart; raw geometry/binning; stream abort and reconnect; cooling; selected camera modes/settings; file-image noise/formats and a missing-file failure; wheel movement and invalid slots; focuser movement and abort; guider pulse reset; and AO corrections/reset.

Coverage still needs to be extended for the refactored semantics: fractional ordinary exposures longer than one second, subsecond precision, finite-stream monotonic progress/completion, abort/restart while queue work is pending, countdown progress while the device queue is occupied, disconnect during each prolonged operation, same-axis guider replacement and simultaneous axes, shared imaging workload during guiding, post-disconnect stale-finalizer checks, file-open short/truncated/invalid-header rollback, reconnect after a failed file connection, and explicit queue/lifecycle assertions. A standalone guider timing benchmark is also required for EAST/WEST/NORTH/SOUTH, representative durations from 20 to 500 ms, warm-up discard, repeated idle and imaging-load samples, and monotonic requested-versus-public-completion statistics. These are software timing endpoints, not physical relay accuracy.

## Baseline

Environment: macOS Darwin 25.6.0 on arm64 Apple hardware, Apple clang 21.0.0. The produced driver and integration-test executable are universal arm64/x86_64 Mach-O binaries.

Commands run before production changes:

```sh
make -B -C indigo_drivers/ccd_simulator -f ../../Makefile.drv all
make -B -C indigo_test build/integration/test_ccd_simulator
indigo_test/build/integration/test_ccd_simulator
```

Result: driver archive, dynamic library and executable built successfully; all 17 existing integration scenarios passed. There were no pre-existing CCD subtree modifications before this record was created. Other unrelated worktree changes are outside this refactoring and must be preserved.

## Hardware-test decision

No hardware testing will be performed. This is a software-only simulator with no physical protocol, vendor SDK or physical relay output. All validation will be simulator-backed. Guider timing results will be labelled as public-property software completion timing and will not be presented as hardware pulse accuracy.

## Atomic plan and results

1. **Complete instructions, source, documentation, build and test audit; establish baseline. — Complete (2026-09-13).** Read the repository, driver and test instructions and relevant migration/queue/class documentation; audited all nine logical devices, current timer/mutex/lifecycle structure, build integration and the 17-test suite. The universal arm64/x86_64 baseline build and 17/17 tests passed with the commands above. Hardware testing was explicitly ruled out.
2. **Resolve repeated-device support in the generator. — Complete (2026-09-13).** After explicit user approval, added optional leading device attribute `id = logical_device_id`. The base type still selects headers, public entry points and `indigo_<type>_*` APIs; the id selects generated callbacks, handlers, templates, pointers, code-block namespaces and master/slave symbol references. Duplicate ids and late ids are rejected. Definitions without `id` retain their historical symbol names and output paths. Generated sources carry an id/type marker so reverse extraction restores repeated blocks. Added the architecture scenario `Repeated device classes use unique ids and survive extraction`, covering two CCD blocks, distinct callbacks/property handlers/templates, master assignment, child-before-master shutdown and regenerated extraction. Updated `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`. Verification: `make -B -C indigo_tools all`, `make -B -C indigo_test build/integration/test_generator_architecture`, and `indigo_test/build/integration/test_generator_architecture`; all 16 generator architecture scenarios passed. The tools build emitted only the pre-existing `indigo_list_usbserial.c` VLA-folding warning.
3. **Create the generator source and migrate lifecycle/property declarations. — Complete (2026-09-13).** A temporary reverse extraction was run under `/tmp` and removed; as expected for the unannotated legacy source, it produced only an incomplete skeleton and was not copied into the repository. Detailed mapping exposed a second generator limitation: the virtual-device connection branch could not report File CCD initialization failure. After explicit user approval, the generator now detects virtual `on_connect` blocks that reference `connection_result` and emits an opt-in success/failure epilogue; false restores disconnected and reports ALERT without defining connected properties, while blocks without the variable retain their historical output. The migration guide documents the contract. `indigo_ccd_simulator.driver` is now the source of truth for all five uniquely identified CCD blocks plus wheel, focuser, guider and AO; it uses one generator-owned shared private-data allocation and child-before-master shutdown. The generated version is 26, higher than the legacy `0x03000019`. A forced generator/driver build succeeded. The resulting metadata test exposed a third opt-in generator gap: generated `SET_DRIVER_INFO` hard-coded `multi_device_support = false`, unlike the legacy simulator. After explicit user approval, the generator gained driver-scope `multi_device_support = true`; omission/default false leaves existing generated metadata unchanged, the supported and unsupported-architecture entry points use the setting, and reverse extraction preserves true. The migration guide and attribute matrix were updated, a dedicated opt-in/default/extraction architecture scenario was added, and all 18 generator architecture scenarios plus all 88 attribute cases passed. The CCD definition now opts in; after regeneration, `driver_info_reports_simulator_metadata` passed.
4. **Refactor prolonged operations to queues and monotonic finalizers. — Complete (2026-09-13).** Ordinary exposure uses `indigo_ccd_exposure_setup()` plus a monotonic `exposure_finalizer`; streaming performs one bounded frame per monotonic `streaming_finalizer`; temperature polling, wheel/focuser motion and guider completion use generated queue callbacks/finalizers. All nine logical devices share the Imager master queue, so the legacy image mutex was removed. An urgent-abort race in which abort could overtake a queued focuser/exposure start was fixed by cancelling those concrete pending start handlers and finalizers; disconnect does the same for CCD, wheel, focuser and guider work. Focuser abort freezes the target at the current position and publishes deterministic ALERT completion. The original star catalogue, polar/periodic error, guide/AO offsets, hot pixels, gain/offset/gamma, defocus, Bahtinov rotation, DSLR JPEG and file-image behaviour was restored inside the serialized queue instead of being reduced to flat noise. Static audit found no mutex, sleep, legacy timer or ad-hoc async call in the `.driver` or generated implementation.
5. **Regenerate and inspect all checked-in generated outputs. — Complete (2026-09-13).** Forced generation and the universal driver build succeeded. A second direct generator run produced identical SHA-1 values for `.c`, `.h` and `_main.c`; after the final cancellation additions their checked-in values are `c016ae17...`, `00bb0d5e...` and `91ec4f15...`. Generated handlers use the master queue, virtual child detach is reverse ordered, file connection uses the fallible connection epilogue, and metadata reports multi-device support.
6. **Complete integration coverage for CCD and embedded device behaviour. — Complete (2026-09-13).** The suite now contains 19 named scenarios. Metadata and lifecycle cover INFO, INIT/enumeration/connect/disconnect/SHUTDOWN; nine compliance cases cover every CCD, wheel, focuser, guider and AO logical device; acquisition cases cover shared countdown, busy overlap rejection, abort/restart, 150 ms and 1.25 s timing, bin/ROI geometry, finite and indefinite streaming, abort and reconnect; cooling covers target/poll/off/reconnect; camera settings cover all guider modes, J2000 correction, Bahtinov rotation and DSLR controls; file tests cover missing, empty, invalid-signature and truncated inputs, all four raw signatures and retry/reconnect; guider covers replacement, simultaneous axes, disconnect while pulsing and fresh post-reconnect pulse; focuser covers movement, abort, disconnect while moving and fresh movement. Transport/SDK/hot-plug failure injection is not applicable to this virtual simulator; codec internals and generic persistence are framework-owned.
7. **Add and run the guider timing benchmark. — Complete (2026-09-13).** Added `bench_ccd_simulator_guider_timing` to `BENCHMARKS`, outside normal tests. It measured EAST/WEST/NORTH/SOUTH at 20/100/500 ms with one discarded warm-up and four recorded samples per combination under idle and concurrent 20 ms Imager streaming: 24 warm-ups and 96 recorded pulses. Endpoints were monotonic request-to-public-OK/reset. Idle worst absolute error was 11.401 ms; shared-imaging worst absolute error was 13.073 ms. Every row reports min/mean/median/p95/p99/max/standard deviation and actual mean. Status was complete. This is simulator software timing, not physical relay-output accuracy.
8. **Register persistent files and verify supported build metadata. — Complete (2026-09-13).** Registered `REFACTOR.md`, `indigo_ccd_simulator.driver` and `indigo_ccd_simulator_data.h` in the CCD simulator Xcode group; the existing integration source remains registered and its Makefile now builds the benchmark. No per-driver Windows project exists. Static portability audit found only portable INDIGO APIs and standard C, no platform branch or POSIX I/O in the `.driver`; Windows, Linux and FreeBSD execution were unavailable and are not claimed.
9. **Run final generation, strict build, sanitizer and architecture validation. — Complete (2026-09-13).** The universal macOS driver build passed. `clang -std=gnu11 -Wall -Wextra -Werror -fsyntax-only` passed for the generated driver. The 19-scenario suite passed under both `arch -arm64` and `arch -x86_64`. An arm64 AddressSanitizer/UndefinedBehaviorSanitizer run initially found an inherited out-of-range double-to-`uint16_t` star-pixel conversion; the generator input now saturates the accumulated pixel before conversion, and the complete 19-scenario sanitized suite then passed. The final native 19-scenario suite also passed after disconnect cancellation was added. Other operating-system execution was unavailable.
10. **Reconcile migration status and final repository audit. — Complete (2026-09-13).** Updated only the `ccd_simulator` status cells in root `MIGRATION_STATUS.md` to Windows/Generator/Async Queues yes and Retested `Sim`, preserving the empty Comment exactly. Version 26 exceeds legacy version 25, generated output is synchronized, all new persistent driver files are registered, no property was added or removed relative to the simulator contract so `PROPERTIES.md` needs no change, and the diff was checked for unrelated scope and avoidable generated artifacts.

## Scenario-to-test mapping

| Capability or risk | Test evidence |
| --- | --- |
| Metadata, repeated lifecycle and shutdown | `driver_info_reports_simulator_metadata`; `simulator_initializes_enumerates_connects_disconnects_and_shuts_down` |
| Imager exposure/countdown/abort/restart/queue overlap | `simulator_countdown_progress_abort_and_restart`; `ccd_imager_passes_compliance_checks` |
| Fractional exposure and finite streaming deadlines | `simulator_fractional_exposure_and_finite_stream_timing` |
| ROI, bins, raw payload geometry | `simulator_raw_geometry_and_bins` |
| Indefinite stream abort/disconnect/reconnect | `simulator_stream_abort_and_reconnect` |
| Cooling and recurring polling lifecycle | `simulator_cooling_target_and_polling` |
| Guider image modes, Bahtinov and DSLR behaviour | `simulator_camera_modes_and_settings`; the three corresponding compliance cases |
| File initialization rollback and raw formats | `simulator_file_noise_formats_and_failure`; `ccd_file_camera_passes_compliance_checks` |
| Wheel/focuser motion and cancellation | `ccd_wheel_passes_compliance_checks`; `ccd_focuser_passes_compliance_checks` |
| Guider directions, replacement, simultaneous axes and disconnect | `ccd_guider_passes_compliance_checks`; `simulator_guider_replacement_and_simultaneous_axes` |
| AO movement, saturation and reset | `ccd_ao_passes_compliance_checks` |

## Final test summary

- Simulated tests run/passed: 76/76 final validation scenario executions (19 native final, 19 arm64, 19 x86_64 and 19 arm64 ASan/UBSan); additionally 18/18 generator architecture scenarios passed. The separate guider benchmark completed 96 measured samples and 24 warm-ups and is not counted as a pass/fail test.
- Hardware tests run/passed: 0/0 (not applicable; explicitly not performed).

## Overlapping guide pulses (2026-09-20)

A guide pulse requested while another pulse on the same axis is still running now replaces the
running one instead of being discarded. The generated change branch dispatched both guide
properties through the BUSY-guarded `INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE` macro, so the
second request never reached the handler; the `indigo_cancel_pending_handler()` call the handler
already carried was unreachable. `GUIDER_GUIDE_RA` and `GUIDER_GUIDE_DEC` now declare
`accept_while_busy = true` and zero both axis items in `on_change_request`, so a reversing request
cannot leave the superseded direction set. This is the same pattern used by every other INDIGO
driver that exposes a guider.

`simulator_guider_replacement_and_simultaneous_axes` was extended with two duration-measuring
cases, because waiting only for the property to leave BUSY passes even when the second request is
discarded: a 2000 ms pulse replaced after 500 ms by a 600 ms pulse in the same direction, and the
same pulse replaced by a 300 ms pulse in the opposite direction. Against the pre-change driver the
first case measures the full 2000 ms and fails; against the fixed driver it measures about 1100 ms.
Partial travel of a replaced pulse is not added to the simulated image offset, because the
finalizer of the superseded pulse never runs; the simulated guiding error therefore lags slightly
behind a real mount when a client replaces pulses mid-flight.

## Silent change refusal during acquisition (DRV-214, 2026-09-21)

Observable impact: in all five camera devices (imager, guider, bahtinov, dslr, file) a `CCD_STREAMING` request arriving while `CCD_EXPOSURE` was BUSY, and the reverse, returned `INDIGO_OK` without publishing anything. A client that waits for a response cannot tell that from a lost request,
and `config_restore` in `indigo_libs/indigo_driver.c` dispatches saved properties one at a time and
waits for each answer, so an unanswered property used to cost every setting after it in the file
(TT-D03 / DRV-213). Found by a static sweep of every `change_property` body in the repository, not by
a failing test.

Root cause: ten `on_change_request` blocks in `indigo_ccd_simulator.driver` refused the cross-property interlock with a bare `return INDIGO_OK;`.

Fix: all ten are now `reject_change` blocks. The simulator matters here beyond its own users: it is what the agent suites drive, so its refusal behaviour is the reference other tests observe. Version 27 -> 28.

Regression test: `simulator_stream_abort_and_reconnect` in `indigo_test/integration/test_ccd_simulator.c` now requests streaming during an exposure and requires `CCD_STREAMING` to reach `INDIGO_ALERT_STATE`. Against the pre-fix driver it fails at that assertion; the suite is 19/19 after the fix. The second `CCD_EXPOSURE` request in the same case is the property's own BUSY guard and is still expected to stay silent.


## Lens defaults regression (2026-09-22)

Source comparison with master found that generated `configure_ccd()` omitted the imager and guider camera lens defaults from legacy attach. The base CCD initialization therefore exposed zero aperture, focal length and physical length with IDLE state. Master sets imager values to 4/12.7/12.7 cm and guider values to 2/5.1/5.1 cm, with OK state. Other camera variants do not set these defaults.

Baseline: `make -C indigo_drivers/ccd_simulator -f ../../Makefile.drv all` passed on macOS. Added assertions to the existing camera compliance scenarios; `indigo_test/build/integration/test_ccd_simulator ccd_imager_passes_compliance_checks` reproduced the missing focal length (1 run, 0 passed).

Plan: restore these assignments in the generator input, increment version 28 to 29, regenerate and build; run the full existing 19-case simulator suite including the expanded lens assertions; verify generation reproducibility and record results. No hardware testing applies to these virtual cameras. No property names, items or visibility change. Automated case count remains 19 / 0.

Completed: restored the legacy value assignments and OK state in `configure_ccd()` for kinds 0 and 1, version 29; regenerated outputs. The driver build and `make -C indigo_test build/integration/test_ccd_simulator` passed. `indigo_test/build/integration/test_ccd_simulator` passed all 19 cases on macOS arm64, including lens values and state for both cameras. A second generator run produced byte-identical C/header/main outputs; `git diff --check` passed. Updated the test record and regenerated `TEST_SUMMARY.md`. No property definitions changed; no new files or case-count changes require project/status registration. Other platforms were not executed.

Final test summary for this fix: simulator regression baseline 1 run / 0 passed (expected); final simulator suite 19 run / 19 passed; hardware 0 run / 0 passed.

## Silent sensor-size change (2026-09-24)

Found during the Linux x86_64 agent test run: the Guider Agent case `selection subframe restore` failed deterministically (expected 1600, got 400). Root cause in this simulator: the `guider_ccd.SIMULATION_SETUP.on_change` block resized `CCD_INFO` and `CCD_FRAME` (value, target and max) and relabelled `CCD_MODE` to the new image size, but published only `SIMULATION_SETUP`. Clients, including the Guider Agent, kept the old 1600x1200 frame, saved it before subframing and sent it back afterwards, where the camera correctly clipped it to the real 400 px width with ALERT.

Fix: when connected, the block now updates `CCD_INFO` and `CCD_FRAME` and redefines `CCD_MODE` so the new labels reach clients. Version 29 to 30; the regenerated `indigo_ccd_simulator.c` differs from the input only by this block and the version. Regression: `integration/test_agent_guider.c` `selection subframe restore`, which now compares the real 400 px frame before guiding with the restored one. The 19-case simulator suite and all agent suites that use this simulator were rerun on Linux x86_64. macOS and Windows were not run for this fix.

Final test summary for this fix: simulator suite 19 run / 19 passed; hardware 0 run / 0 passed.

## Mount simulator integration (2026-09-26)

Reported: `CCD Guider Simulator` no longer changed the generated star field when `Mount Simulator` moved.

Root cause: the master driver followed the mount through `CCD_SET_FITS_HEADER`. The Mount Agent sent `OBJCTRA`, `OBJCTDEC`, `PIERSIDE`, `SITELAT` and `SITELONG` to the related Imager/Guider Agent, which forwarded them to the camera, and the camera parsed them into its guider-image setup. The generator migration (`6b6b9bd69`) did not carry that `CCD_SET_FITS_HEADER` branch into the `.driver` source, so `SIMULATION_SETUP` `RA`/`DEC`/`SIDE_OF_PIER`/`LAT`/`LONG` changed only on client requests. The old path also needed both agents to be related, parsed whole arcseconds only and read Dec between -1° and 0° as positive.

Fix (version 31 to 32): the FITS-header path is not restored. The camera and the mount simulator now share state through `libindigo` (`indigo_set_simulated_mount_state()`, `indigo_get_simulated_mount_state()` and `indigo_simulated_mount_guide()` in `indigo_mount_driver.h`). The library is the only state both drivers share: they are separate static archives or `dlopen()`ed modules, and those are loaded without `RTLD_GLOBAL`.

- `search_stars()` first takes the published physical (raw) pointing, epoch, site and side of pier into `SIMULATION_SETUP` and publishes it when it changed. Epoch 0 selects the JNow catalogue positions and any other epoch selects J2000. Without a connected mount simulator, the client's own values stay.
- Star positions are kept as `double` instead of truncated to whole pixels. A 7° field over 1200 px is about 21″ per pixel, so a real guide pulse moves stars by a fraction of a pixel. With truncation, the image would move only in whole-pixel steps.
- While a mount simulator is connected, `CCD Guider Simulator (guider)` pulses move the mount at the physical rate (`GUIDER_RATE` % of sidereal) through `indigo_simulated_mount_guide()`, and the image follows the mount. Without a mount, the pixel-offset model (`GUIDER_GUIDE_SCALE`) is unchanged.
- The off-by-one `star_count++ == GUIDER_MAX_STARS` wrote one element past the star arrays when the field held more than `GUIDER_MAX_STARS` stars. It is now `++star_count == GUIDER_MAX_STARS`.

Limitation: sharing needs both drivers in one process. With standalone driver executables or drivers on different servers, the camera keeps the client's setup.

Regression tests in `integration/test_ccd_simulator.c` (the mount simulator archive is now linked into this binary):

- `simulator_guider_camera_follows_simulated_mount`: a published pointing places Betelgeuse in the frame centre and is reflected in `SIMULATION_SETUP`. A Dec move of 0.5 px moves the star centroid by 0.487 px. JNow and the west side of the pier are taken over. Once the mount is withdrawn, the client's RA/Dec stay.
- `simulator_guider_camera_follows_mount_simulator_guiding`: the real `Mount Simulator` is synced to Betelgeuse. A 3 s north pulse of `Mount Simulator (guider)` moves the star 1.035 px (expected 1.074 px), and a 3 s west pulse moves it 1.029 px (expected 1.065 px). 3 s south and east pulses of `CCD Guider Simulator (guider)` return it within 0.06 px, and the mount keeps that position after its next update.

Validation on macOS arm64: `test_ccd_simulator` 21/21 passed, and the two new cases were repeated five times with identical results. Linux and Windows were not run.

Final test summary for this change: simulator suite 21 run / 21 passed; hardware 0 run / 0 passed.
