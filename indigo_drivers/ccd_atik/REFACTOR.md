# Atik CCD generator migration

Date: 2026-09-12. Baseline: `0b3484dd225ee01f52ac16b93b7a01e48d076d29`, clean workspace, driver `0x0300001F` (1055 lines).

Status: software migration and fake SDK acceptance completed; physical acceptance remains pending. Physical acceptance is deferred until software acceptance, with Atik Titan, Atik One, Atik 11000A and Atik Horizon. No hardware results are implied by historical README entries.

## Scope and references

Migrate only ccd_atik and its tests/documentation/project entries. Follow ccd_playerone, ccd_asi and ccd_sx REFACTOR patterns and current generator source, DRIVER_GENERATOR_MIGRATION.md, DRIVER_DEVELOPMENT_BASICS.md, DEVELOPMENT.md, MAKEFILES.md, README.md, TESTING.md, indigo_test/AGENTS.md and DRIVER_TESTING_RULES.md (CCD, guider and wheel). Bundled AtikCameras.h and AtikDefs.h are the executable SDK contract; vendor PDF/CHM documentation needs comparison for model-specific option payloads and historical SDK workarounds. Do not alter the SDK or generator without separately approved generator changes. Keep this migration inventory separate from incremental REVIEW baselines.

## Existing implementation inventory

Line references are to baseline C, not regenerated output.

| Lines | Responsibility and findings |
| --- | --- |
| 28–90 | Version 31; two USB VIDs 0x20e7/0x04b4, four ST4 bit masks, shared handle/index/64-byte serial/device_count/buffer, exposure/temperature/single guide timers; two custom properties lack X_ prefix. |
| 94–105 | Global debug suppression changes across cameras/threads. Remove shared mutable suppression while preserving SDK logging. |
| 108–137 | Exposure callback sleeps remaining SDK time then polls ImageReady forever. No CameraState error/watchdog, ImageFailed check, image-pointer check or dimension/capacity validation before memcpy. Pixels are 16-bit SDK processed mono/raw; no driver Bayer metadata or streaming implementation. |
| 139–165 | Five-second temperature polling; separate measured/target, one-degree settling tolerance, 0.1-degree rounding; unchecked cooling range divisor and ignored poll failures. Historic temperature gating cannot prevent all concurrent SDK access. |
| 167–318 | CCD connect manually increments shared refs before successful open; Properties failure closes a sibling's shared handle. Uses unvalidated max bins/log2/count and uninitialized outputs. Sensor dimensions 3354x2529 are cropped to 3326x2504 (383 family workaround). Allocates full RAW16+FITS buffer. SDK_2020_06_23 workarounds ignore temperature/cooling status; need explicit documented compatibility treatment. Heater, presets, gain and offset readbacks ignore payload lengths and sometimes overwrite ALERT with OK. |
| 319–390 | Attach defaults: exposure min .001 s, 16 bits, RW bins up to four, visible read mode, optional custom heater/presets. Enumerates custom properties while connected. Disconnect deletes custom properties only on final shared close, stops exposure with racing timer, warms on CCD-last-close. |
| 392–559 | Bus callback performs SDK controls directly; acquisition flush polling blocks callback indefinitely; ignored setup/cooling failures. Gain/offset int writes, preset short writes, six-byte min/max/value readback. Preset visibility changes before successful write, no rollback. Binning updates mode selection, otherwise base CCD frame/mode behavior. Custom heater readback on write failure is unchecked. No custom CONFIG persistence. |
| 563–708 | Optional guider uses shared count/handle. Guider disconnect incorrectly frees CCD buffer, may leave physical relays active. Single timer clears both axes: overlapping pulses truncate or extend the other axis. SDK guide failures ignored. Preserve four direction masks and replacement/zero semantics with independent finalizers. |
| 711–838 | Optional integrated wheel uses zero-based SDK slots and one-based public slots/names/offsets. Motion polls .5 s, ignores move errors, clamps bad SDK positions to zero, has no watchdog or owned timer cancellation. Failed initial info leaves ambiguous connection state. |
| 843–1055 | Manual ten-logical-slot array/mutex, .5 s unowned discovery timers, serial matching and USB callbacks; attach and overflow failures leak/orphan resources. Arrival clears indices but only updates first matching logical device. Removal marks only first serial match, potentially detaching surviving siblings. No camera-only SDK filter. Partial callback registration and last_action rollback incomplete. Shutdown calls ArtemisShutdown. |

SDK details: name/serial functions require buffers of 100 bytes (baseline has smaller buffers). Index is enumeration position, not identity; resolve serial again at open after reordering. ArtemisDeviceIsCamera separates stand-alone wheels. ImageReady must precede ImageFailed and image access. SDK owns returned pixel storage; copy while serialized before another exposure. Header temperature prose contradicts itself (hundredths/tenths/degrees); preserve legacy hundredths conversion until checked with hardware/vendor guide. Preserve preview/dark and unbinned subframe ordering, fractional seconds and 383 crop. Do not add new SDK capabilities (fast streaming, raw 8-bit, external trigger, GPIO, reset, name writes) merely because the header offers them.

## Public property mapping

| Baseline | Target | Contract |
| --- | --- | --- |
| ATIK_PRESETS | X_PRESETS | CCD main, RW one-of-many CUSTOM/LOW/MED/HIGH; model capability; preset 0 exposes standard CCD_GAIN/CCD_OFFSET with SDK ranges/current value. |
| ATIK_WINDOW_HEATER | X_WINDOW_HEATER | CCD main, RW number POWER 0..255 step 1; heater camera flag; checked readback. |

Standard properties keep names. CCD_READ_MODE visible; CCD_STREAMING hidden. Optional temperature RO or RW, cooler and power according to discovered support; all visibility/count/range/permission changes must reset on reconnect. Wheel slot count updates names/offsets. Guider and wheel attach only if advertised. Custom names intentionally break old client scripts; document names in README and PROPERTIES with .driver source mapping. No undocumented aliases or added persistence.

## Target ownership

Use `driver atik`, generated SDK discovery/registration/arrays/reference counting and default MAX_DEVICES. Register any VID and filter both existing VIDs in sdk.plug (single VID syntax cannot express a list); no generator extension required. Deduplicate by complete checked serial, reject incomplete discovery, match removal by SDK presence, and use bounded generated retries. Model/serial formatting uses full SDK buffers and bounded public names. Capability-gated guider/wheel share the CCD master and SDK session.

`atik_open()` acquires global lock and handle transactionally; `atik_close()` closes exactly one successful session. Generated count owns references. CCD buffer belongs to CCD connection, never to guider. Lock master around driver-queue connect/disconnect initialization because property workers use another queue; prove serialization with instrumented SDK gates. Device workers use shared queue; queued start/finalizers/polling cancel before disconnect/close. Exposures use indigo_ccd_exposure_setup, exact requested duration, bounded flush/readiness finalizers and failure cleanup. Independent RA/DEC finalizers change only their mask. Wheel uses bounded progress polling and checked slots. No blocking wait loops or OS-specific production code.

## Atomic steps and progress

Each step is a reviewable workspace checkpoint; no automatic commits requested. Update evidence here after each checkpoint.

1. **Inventory and baseline** — COMPLETE for source/build, SDK PDF audit completed: sensor/target units are 1/100 degree; warm-up is explicitly required at end of operation. The PDF does not specify Horizon option payload layouts; preserve existing payload sizes with checked actual lengths and verify on Horizon.
   - Read full baseline, public properties, SDK declarations and reference migrations.
   - `make -B -C indigo_drivers/ccd_atik -f ../../Makefile.drv` passed on macOS arm64, Apple clang 21.0.0. Log `/tmp/atik-baseline-build.log`.
2. **Seed fake SDK baseline** — COMPLETE. Three separately named CCD/guider/wheel cases passed against the unchanged driver, then against generated output; logs `/tmp/atik-baseline-test.log` and `/tmp/atik-generated-test.log`.
   - Separate production translation unit, fake Artemis/libusb boundary and public bus client with real framework queues. Capture camera-only/guider/wheel profiles, commands, RAW16 noise and normal lifecycle; record baseline defects rather than asserting them as desired behavior.
3. **Implement queue workers and complete .driver cutover** — COMPLETE. Version 32, generated lifecycle and bounded workers passed normal and sanitizer acceptance. No generator implementation changes.
   - Complete source of truth atomically; checked initialization/control/image replies, nonblocking exposure/guide/wheel finalizers, buffer ownership, generated hotplug and reference count. Version at least 32; preserve license and extend year to 2026. No partial generated production cutover.
4. **Synchronize properties and build projects** — COMPLETE. X_ names, source mapping, README, Xcode and Visual Studio entries updated; XML parses and repeated generation produces byte-identical C/header/main.
   - Generator output .c/.h/_main.c, X_ mapping, README, Xcode and Visual Studio source listing; compare second generation byte-for-byte, forced production build and targeted tests.
5. **Complete fake acceptance matrix** — COMPLETE for the 36 scenario groups mapped below.
   - Named tests mapped below; normal and ASan/UBSan builds, bounded failure/race recovery and no after-close SDK calls. Check generated code and whitespace, then test-clean.
6. **Software completion records** — COMPLETE. CHANGES and migration status updated; hardware/platform limits below.
   - CHANGES scenario mapping; update only MIGRATION_STATUS status columns, preserve Comment exactly; list unverified Linux/Windows and hardware gaps here.
7. **Physical acceptance with user** — DEFERRED until 1–6 pass.
   - Opt-in public-client tests for each Titan, One, 11000A, Horizon: discover real capabilities, full/ROI/binned RAW16, .001/.1/1.5/2.5/16.5 s, bias/dark/light/darkflat/flat, read modes, abort/restart, cooldown/power/warm-up, heater, Horizon presets/custom gain/offset, each supported ST4 direction/overlap and integrated wheel positions.
   - All shared connection orders, repeated disconnect/reconnect, real library unload/reload and multiple-camera identity; actual removal idle/active followed by replug/reacquire. Restore controls/temperature/wheel selection where possible. Record actual SDK/model/serial and outcomes in TESTING.md; do not infer optical shutter or electrical ST4 accuracy from software completion.

## Acceptance design (implemented mapping and evidence follow)

| Named area | Assertions / failure boundaries |
| --- | --- |
| metadata_profiles | Before/after/disconnect enumeration; names, interfaces, X_ properties/items, modes/ranges, optional CCD/guider/wheel; unsupported controls hidden; reconnect rebuild. |
| lifecycle | INIT/SHUTDOWN/repeat, connected shutdown rejection, all three interfaces alone and connection permutations, sibling survival, last close and lock balance. |
| discovery | Two VIDs/unrelated devices, camera-only filter, duplicate/reordered/burst events, serial/name limits/failures, incomplete scan, delayed discovery, master/slave attach and queue/registration failures, capacity/refill, balanced USB references. |
| initialization | Global lock/open/Properties/max bins/temp/cooling/heater/preset read failures; malformed SDK outputs and lengths; rollback then fresh connect. |
| acquisition | Full/ROI/asymmetric bin/383 crop; deterministic coordinate-addressable noise; read mode/dark mapping; min/subsecond/fractional exposures/countdown, BUSY conflicts, flushing/readiness, ImageFailed/state errors/null buffer/malformed geometry, setup/start/stop failures and fresh exposure. |
| cooling_controls | Unit conversion, measured versus target, settle/power/zero span, ON/OFF failures and poll recovery, cancellation; heater limits/readback failures; each preset/custom gain/offset, short/invalid replies, atomic visibility and recovery. |
| guide | Four masks, zero/stop, replacement/reversal, independent axes, CCD acquisition coexistence, start/stop failures and cancellation, guide-only and sibling survival. Record timing statistics at fake SDK ON/OFF for short/long pulses idle/under acquisition. |
| wheel | Slot count and one-based mapping, initial moving/unknown/malformed positions, first/intermediate/last/same-slot moves, BUSY overlap, start/poll/timeout failures, disconnect motion and reconnect recovery. |
| races | Deterministic gates around start/readout/config/connect; urgent abort overtakes queued start; disconnect/removal pending exposure/pulse/wheel/poll; no updates after detach or SDK calls after close; accepted shutdown drains discovery. |

Non-applicable: no driver streaming, Bayer conversion, suffix writes, SDK callback image delivery, calibration, wheel direction/speed, focuser/rotator, custom persistence or direct serial protocol. Framework image encoders/upload formats, numeric validation and generic slot-name/config storage are outside driver-specific tests. Applicable SDK command/return failures are tested at the actual boundary.

## Completed software acceptance

The source of truth is `indigo_ccd_atik.driver`, version 32 (`0x03000020`), higher than baseline 31. The generator owns lifecycle, property ownership, shared count and USB queue/ref cleanup. SDK/property calls serialize on the physical camera queue; connection initialization and disconnect take the master lock across their SDK/buffer work. There are no old exposure/temperature/guider timer pointers, manual device_count, platform-specific driver code or MAX_DEVICES override.

Important differences from baseline:

- Generator default is **five logical slots**, not the old ten. CCD + guider + wheel consumes three; connect the four acceptance cameras in appropriate batches. Full capacity/refill is covered. Increasing the default/overriding it was not requested.
- Discovery uses SDK serial identity and 100-byte SDK output buffers, re-resolves the current SDK index on each open, filters non-cameras and both original VIDs, and treats incomplete serial enumeration as inconclusive on removal. Generated retries handle delayed SDK visibility.
- Exposure setup/flush/readiness now yields, with 30-second flush and 120-second post-exposure readout allowances. Exact fractional duration remains independent of the shared displayed countdown. SDK ImageFailed, NULL buffer, geometry/binning and capacity are checked before copying; failed stop remains retryable. No unbounded polling or sleeping acquisition callback remains. Temperature polling remains allowed during integration and is deferred during SDK download, preserving the original exclusion.
- Guide axes finish independently; same-axis requests replace the pending pulse, including zero/stop. A failed relay-off publishes ALERT and can recover on a subsequent request or disconnect; software cannot guarantee an electrical OFF when the SDK itself fails.
- CCD disconnect owns its buffer, stops acquisition and warms a supported cooler without closing connected siblings. Guider disconnect no longer frees CCD memory. Wheel operations check SDK positions and command errors and have a 60-second progress deadline.
- The historical 3354x2529 -> 3326x2504 crop, SDK preview/dark setup, RAW16 handoff and option write sizes (16-bit preset, int gain/offset) are preserved. The old unconditional acceptance of failed temperature/cooling calls is replaced by checked current-SDK status/outputs; NOT_IMPLEMENTED remains an optional-capability fallback. This compatibility difference particularly needs real-camera verification.
- `X_PRESETS` and `X_WINDOW_HEATER` replace the old custom names. Failed control writes/readbacks retain valid known values; custom mode only exposes gain/offset after checked six-byte range/current replies. Standard CONFIG gain/offset restore reaches SDK setters; custom preset/heater persistence was not added.

### Scenario-to-test map

All tests are in `indigo_test/integration/test_ccd_atik_sdk.c`, compiled separately from production C and linked with real bus, CCD/wheel/guider bases, queues, countdown and image handling. Only SDK/USB/discovery/global lock boundaries and test-only clock/delay/queue instrumentation are substituted. A separately compiled framework driver object isolates CONFIG files in a test-owned mkdtemp directory. No real SDK, USB inventory, server or network socket is used.

| Area | Passing named cases |
| --- | --- |
| Baseline/public contract | baseline_camera, baseline_guider, baseline_wheel, metadata_profiles |
| Three-interface lifecycle/rollback | shared_lifecycle (six connect permutations and sibling survival), initialization_failures, malformed_initialization |
| Discovery | discovery_filters_and_strings, delayed_discovery_and_shutdown, capacity_and_survivors, optional_slave_attach_failure, discovery_identity, discovery_rollback |
| Acquisition/RAW16 geometry | acquisition_errors, geometry_and_payload, durations_and_frame_types, crop_383_image, final_mode_bin_synchronization |
| Controls/configuration/cooling | cooling_and_controls, poll_errors_and_cancellation, preset_readback_failures, final_configuration_gain_offset |
| Guide | guide_axes_and_replacement, guide_duration_measurements, final_guide_off_failure_recovers |
| Wheel | wheel_errors, initial_wheel_moving, final_wheel_busy_preserves_target |
| Timeout/abort/concurrency | abort_and_busy, deadlines_and_recovery, pending_start_abort, readout_disconnect, active_removal, countdown_with_occupied_queue, stop_failure_recovery, final_readout_state_failure |

Fake pixel data is coordinate-addressable deterministic noise from `ccd_test_noise.h`, never a simulator photograph. SDK instrumentation counts entry/exit to reject overlapping per-handle calls, calls from the bus thread and calls after close. Final teardown asserts balanced locks, logical attachments and USB references, and no property updates after detach. Delay gates exercise queued-start abort and disconnect during image access; test-only shifted monotonic deadlines cover bounded failures without waiting minutes. Configuration SAVE/LOAD initially failed because the inherited default image directory did not exist; setting CCD_LOCAL_MODE.DIR to the test-owned directory fixed the test without a production change.

Guide timing uses fake ArtemisGuidePort ON/OFF entry timestamps, four directions, 20/50/100/250/500 ms, two repetitions, discarded warm-ups, idle and during acquisition. Reports contain all 80 requested/actual samples, signed ms/percentage errors and min/mean/median/p95/p99/max/stddev/max-absolute statistics. No host-dependent tight timing threshold is asserted. These are host/SDK-boundary measurements, not electrical ST4 measurements.

### Validation evidence

- Forced normal production build: `make -B -C indigo_drivers/ccd_atik -f ../../Makefile.drv`, macOS arm64 host, universal arm64/x86_64 archive/dylib/executable; `/tmp/atik-final-production-build.log`. Linker retains the baseline deployment warning: SDK dylib targets macOS 10.15, project requests 10.10. No new compiler warning.
- Final `make -C indigo_test test-ccd-atik-sdk`: all 36 groups passed together after the download-phase temperature exclusion was retained; `/tmp/atik-36-tests.log`. Additional metadata/interface/item-contract assertions passed a targeted rerun (`/tmp/atik-metadata-test.log`).
- Final ASan/UBSan arm64: all 36 groups passed together; `/tmp/atik-asan-36-tests.log`. Additional metadata assertions passed a targeted sanitizer rerun (`/tmp/atik-asan-metadata-test.log`). Production driver, test and isolated framework driver object instrumented; prebuilt framework/dependency archives excluded. LeakSanitizer was disabled on this macOS run; resource-balance assertions are not a measured whole-process leak audit.
- Two generator runs produced identical SHA-256 digests for C/header/main; Visual Studio XML parses. `git diff --check` passed. Project entries include every new persistent file. No SDK or generator source was modified.
- Hardware client `indigo_test/hardware/test_ccd_atik_hw.c` compiled only. It requires `--run`, library/entry arguments and an explicit `INDIGO_TEST_DEVICE` substring; target `test-ccd-atik-hw` is excluded from ordinary integration tests. `ATIK_HW_CASE` selects exposure/geometry/settings/presets/wheel/abort/guide. No physical test has run.

This is scenario acceptance, not measured 100% line/branch coverage. No Linux/Windows build/runtime or vendor-SDK runtime acceptance is claimed. SDK-internal stalls cannot be preempted by INDIGO queue priorities. Model-specific image offsets, historical temperature/cooling return codes, Horizon option layouts, actual cooling/heater response and removal during SDK-internal download remain physical acceptance obligations. Hardware client covers reusable software workflows; cooling, physical removal/replug and any model-specific recovery are to be completed with the user during step 7. No TESTING.md success entry has been added prematurely.
