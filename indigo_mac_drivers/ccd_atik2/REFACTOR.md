# Atik2 CCD generator migration

Date: 2026-09-12. Baseline: `725526e56de5d897e2180aedb888cb551c48889f`, driver `0x0007` (721 lines).

Status: migration is complete with driver version 9. All 24 hardware-free scenario groups pass in normal and ASan/UBSan runs; generated files are reproducible and the universal production build passes. A complete opt-in physical acceptance run passed on an Atik One with its integrated five-slot wheel and adjustable cooling. The historical README list is not acceptance evidence and was not changed.

## Scope and references

Migrate only `indigo_mac_drivers/ccd_atik2` and its tests, documentation and project entries. Follow the completed `ccd_atik` migration pattern, `DRIVER_GENERATOR_MIGRATION.md`, `DRIVER_DEVELOPMENT_BASICS.md`, `DEVELOPMENT.md`, `MAKEFILES.md`, root and test `AGENTS.md`, and `DRIVER_TESTING_RULES.md` for CCD, guider and wheel devices. The bundled `libatik.h` and library are the available transport contract. Do not modify `libatik`, the driver generator or the separate `ccd_atik` driver.

## Baseline implementation inventory

Line references describe the 721-line baseline source at the commit above.

| Lines | Responsibility and findings |
| --- | --- |
| 26–61 | Version 7; manually owned shared SDK context, USB reference, connection count, six timer pointers, image buffer, cooler state, shared guider mask and wheel positions. Copyright stopped at 2024. |
| 65–119 | Separate short/long timer callbacks duplicate geometry and readout logic. Long exposure uses a pre-exposure timer; image dimensions and buffer capacity are not validated before publishing. Fractional countdown is manually reduced to zero instead of using the shared monotonic CCD countdown. |
| 121–149 | Temperature timer calls `libatik_set_cooler()` on every successful poll, ignores its result and trusts unvalidated temperature/power outputs. It is gated by mutable state to avoid overlap with readout. |
| 151–314 | Hand-written CCD attach/connect/change/detach. Shared count is incremented before a successful open; SDK context fields and allocation size are trusted; allocation failure asserts. Cooler initialization errors are ignored. Exposure setup and abort calls are partly unchecked; the long start result is ignored; timer cancellation misses the pre-exposure timer. Only equal 1x1/2x2/4x4 binning is accepted despite SDK-reported maxima. |
| 318–436 | One guider timer owns both axes, so overlapping RA/DEC pulses cancel or clear each other. Relay writes and relay-off failures are ignored. Shared connection boilerplate is duplicated. |
| 440–532 | Wheel status/move results and slot bounds are not validated; motion has no deadline. Shared connection boilerplate is duplicated. |
| 539–674 | Manual hot-plug arrays and asynchronous attach calls. Capacity can leave only some logical siblings attached; removal/detach races are not serialized on a driver queue. Identity is a raw USB pointer and names include the USB path. |
| 676–721 | Manual two-VID callback lifecycle. Partial callback registration rollback and queued hot-plug shutdown synchronization are absent. Driver conflicts with `indigo_ccd_atik`. |

## Refactor design

- `.driver` is the source of truth; generated `.c`, `.h` and `_main.c` remain synchronized.
- Generator-owned SDK hot-plug discovery reserves capacity for the CCD and optional guider/wheel as one unit, retains the USB device exactly once, serializes discovery/connect/detach on the driver queue and provides shared open-count ownership.
- `atik2_open()` acquires the global lock transactionally and releases it on every failed open. `atik2_close()` owns the final SDK close and unlock.
- CCD connection validates dimensions, pixel sizes, minimum exposure, SDK bin limits, cooler samples and overflow-safe RAW16 buffer size. It advertises all common power-of-two equal-bin modes supported by both axes.
- Exposure requests snapshot exact duration and aligned geometry. Short SDK acquisitions remain a bounded synchronous transport operation below one second; long integrations start once and complete through `exposure_finalizer`. The framework monotonic countdown is used for both paths. Readout dimensions and capacity are checked before image publication.
- Cooling is changed only by property requests. Five-second polling validates independent status, power and temperature outputs and is skipped during acquisition.
- RA and DEC have independent priority finalizers and replacement/zero-stop semantics while preserving the combined relay mask.
- Wheel commands are 1-based as defined by `libatik`; connection and polls validate count/position, and movement has a 60-second monotonic deadline. Legacy `libatik` 1.9 returns false even after sending a successful wheel command and reports position zero while motion is active, so completion is determined by bounded readback: zero is accepted only as the moving sentinel and the last valid public slot is retained.

## Property inventory

The driver adds no custom properties. It exposes standard CCD properties plus capability-dependent `CCD_COOLER`, `CCD_TEMPERATURE` and `CCD_COOLER_POWER`, optional standard guider properties, and optional standard wheel properties. No `indigo_docs/PROPERTIES.md` change is required.

## Automated acceptance map

The production driver is compiled separately from the test and linked against fake `libatik` and USB boundaries while retaining the real INDIGO bus, device/master queues, timers, property base classes, countdown and image processing.

| Scenario | Named test/evidence | Status |
| --- | --- | --- |
| Metadata, version, three interface bits and before/after-connect property visibility | `metadata_and_properties` | Passed |
| CCD-only, guider-only, wheel-only and all six shared connection orders; one open/final close | `shared_lifecycle`, `capability_profiles` | Passed |
| Lock/open rollback on each logical interface; sibling survives CCD initialization failure | `initialization_failures`, `shared_connection_rollback` | Passed |
| Invalid dimensions, pixel sizes, minimum exposure, bin limits, cooler power/temperature and recovery | `malformed_initialization` | Passed |
| Short/long/fractional exposure, bias minimum, five frame types and read mode | `exposure_modes_and_timing` | Passed |
| Full frame, aligned ROI, all advertised bins, RAW16 dimensions and deterministic per-pixel payload | `geometry_and_payload`, `mode_and_bin_contract` | Passed |
| Start/read failure, malformed image dimensions, abort/restart and busy bin rejection | `acquisition_failures_and_abort` | Passed |
| Abort SDK failure/recovery, readout deadline and abort of a queued-but-not-started exposure | `abort_failure_and_deadline`, `pending_start_abort` | Passed |
| Disconnect and USB removal while pixel readout is blocked; serialization, close ordering, replug and recovery | `readout_disconnect_and_removal` | Passed |
| Shared monotonic fractional countdown while the master handler queue is occupied | `countdown_with_occupied_queue` | Passed |
| Cooler capability profiles, setpoint/on/off, set/poll failures, recovery, acquisition exclusion and disconnect cancellation | `cooling_and_polling`, `cooling_failures_and_exclusion` | Passed |
| Four guide directions, independent axes, same-axis replacement/zero, connection/on/off failures and disconnect relay clearing | `guider_operations`, `guider_failures_and_disconnect` | Passed |
| Wheel connection validation, slot boundaries, already-selected slot, false setter result with successful motion, zero moving sentinel, BUSY overlap rejection, genuine command timeout, poll failure and reconnect count rebuild | `wheel_operations`, `wheel_initialization_failures` | Passed |
| Duplicate/burst arrival, capacity, active removal/replug and survivor operation | `hotplug_and_capacity` | Passed |
| Wrong VID, camera match failure and master/guider/wheel partial-attach rollback | `hotplug_filters_and_attach_rollback` | Passed |
| Failed callback/queue registration, descriptor failure, shutdown while connected and repeated INIT/SHUTDOWN | `driver_lifecycle_and_races`, `repeated_init_shutdown` | Passed |
| Resource/race instrumentation: no SDK call after close, overlapping SDK entry, SDK work on bus thread, unbalanced lock/USB reference or update after detach | asserted after every group | Passed |

Non-applicable unless the implementation changes: streaming, Bayer/color conversion, gain/offset/custom controls, device-name writes, calibration, wheel direction/speed, focuser/rotator and device-side slot-name persistence. Generic codecs, upload destinations, generic numeric validation and framework configuration storage are outside this driver suite.

## Validation log

- 2026-09-12, inventory: compared the 721-line version-7 source with the completed `ccd_atik` migration, generator guide, driver lifecycle/queue documentation, CCD/guider/wheel test standard, bundled `libatik.h`, macOS review findings and project/build entries. No property additions/removals were found, so `PROPERTIES.md` needs no edit.
- 2026-09-12, generator input: created `indigo_ccd_atik2.driver` and regenerated C/header/main with the unchanged repository generator. First compile exposed the missing `indigo_client.h` declaration for the retained conflicting-driver check; adding the documented header fixed it without a generator change.
- 2026-09-12, production build: universal arm64/x86_64 `make -f ../../Makefile.drv all` passed. Linker emitted only the pre-existing warning that two x86_64 `libatik.a` members target macOS 10.12 while the project minimum is 10.10.
- 2026-09-12, first fake suite: implemented 11 initial groups. The first run found four test/harness timing mistakes rather than production failures: switch `CCD_MODE` was sent through a numeric helper, cooler assertions observed the initial BUSY update before the queued SDK call, capacity assumed ten rather than the generator's five logical slots, and wheel timeout shifted the fake clock before the queued handler established its deadline. Each harness issue was corrected and its group passed.
- 2026-09-12, acquisition audit: inspection of regenerated dispatch showed urgent abort cancelled the delayed finalizer but not a queued `ccd_exposure_handler`. Added the same explicit pending-start cancellation used by `ccd_atik`; `pending_start_abort` proves zero SDK starts before recovery.
- 2026-09-12, complete fake matrix: expanded to 24 independent groups covering every implemented SDK boundary and every applicable CCD/guider/wheel/lifecycle row above. One full normal arm64 run passed all 24 groups. Every fixture also passed the common zero-leak/zero-race assertions for locks, USB references, after-close calls, bus-thread SDK calls, overlapping SDK entry and after-detach updates. The expected queue-creation error log is emitted only by the injected `driver_lifecycle_and_races` failure case.
- 2026-09-12, project/docs: added `.driver`, generated main, this `REFACTOR.md` and the fake test to the Xcode project groups; added the normal integration and opt-in hardware target and updated the migration row. A proposed README clarification was reverted, and root `AGENTS.md` now requires explicit user approval before any README change.
- 2026-09-12, readout-removal audit: added an SDK gate around `libatik_read_pixels` and requested both disconnect and physical removal while readout was blocked. The first run corrected a harness expectation: generator removal detaches the idle guider and wheel immediately, while the CCD and SDK context remain until the serialized readout finishes. No SDK overlap, premature close or post-detach update occurred.
- 2026-09-12, repeatability audit: a full-matrix rerun exposed a timing-sensitive guider replacement test whose 100 ms pulse could legitimately complete under a loaded sanitizer/build host before the zero-duration replacement was submitted. The replacement window was enlarged to 500 ms without changing production code; the test still asserts immediate cancellation and does not sleep for that interval.
- 2026-09-12, final normal matrix: recompiled the production driver boundary and reran all 24 groups after adding master, guider and wheel attach-failure rollback cases plus blocked-readout disconnect/removal serialization and wheel BUSY overlap rejection; all passed.
- 2026-09-12, sanitizers: rebuilt the arm64 production driver object and complete test with AddressSanitizer and UndefinedBehaviorSanitizer at `-O1 -fno-omit-frame-pointer`; all 24 groups passed without a sanitizer diagnostic. LeakSanitizer was disabled and prebuilt framework/vendor libraries were not instrumented.
- 2026-09-12, reproducibility: generated `.c`, `.h` and `_main.c` from the final `.driver` in an isolated temporary directory and byte-compared all three outputs; all matched.
- 2026-09-12, final production build: rebuilt the arm64/x86_64 archive, dylib and executable successfully. The only diagnostics were the known macOS deployment warning for two x86_64 members of the bundled `libatik.a` (built for 10.12 while the project requests 10.10).
- 2026-09-12, final verification: `plutil -lint indigo.xcodeproj/project.pbxproj` and scoped `git diff --check` passed, `README.md` has no diff, the final normal and ASan/UBSan matrices passed all 24 groups, and `make -C indigo_test test-clean` removed generated test artifacts.
- 2026-09-12, first physical run: the sandboxed attempt discovered no camera and stopped before connection. The authorized physical run found Atik One USB `20e7:df3c`, serial `0869558227`, firmware `0.40`, using legacy `libatik` 1.9. CCD exposure, frame-type, ROI, binning, read-mode and cooling phases passed, but version 8 rejected the first wheel move because the SDK setter reported false.
- 2026-09-12, physical wheel diagnosis and fix: arm64 disassembly of the bundled library showed `ic24_set_filter_wheel()` has no true-return path. Version 9 initiates motion regardless of that unreliable value and uses validated readback plus the deadline. A second run established that readback position zero is the SDK's moving sentinel; the finalizer now accepts it only during motion and retains the last valid public slot. Fake SDK coverage reproduces both quirks plus a genuinely unaccepted command. The isolated physical wheel test then passed slots 1–5 and restoration to 1.
- 2026-09-12, hardware harness correction: the first complete version-9 run reached every functional phase, but the harness restored the original FITS format after geometry and then incorrectly validated later FITS images as RAW. The harness now retains the explicitly selected RAW format until cleanup and prints format/signature details for invalid frames. Isolated geometry, read-mode, cooling and abort/restart diagnostics all passed.
- 2026-09-12, final Atik One acceptance: the complete run from 22:52–23:01 passed with valid RAW16 data. Requested exposures `.001/.1/1.5/2.5/16.5` seconds completed in `10.823/10.928/12.654/13.675/27.663` seconds including readout; all five frame types, `(16,16) 128 x 128` ROI, bins 1–128, HIGH_SPEED/LOW_NOISE, cooling `21.9 -> 18.9 °C` with nonzero power and a cooled image, wheel slots `1–5–1`, abort/restart, disconnect/reconnect, shutdown, `dlclose/dlopen` and a fresh exposure passed. Cleanup restored wheel slot 1, the original cooler selection and temperature target, and disconnected cleanly. Log: `/private/tmp/atik-one-ccd-atik2-v9-full-final.log`.
- 2026-09-12, post-hardware regression: final generated outputs byte-match an isolated regeneration; the universal production build passes with only the pre-existing x86_64 macOS deployment warning; all 24 fake groups pass in both the normal universal and arm64 ASan/UBSan builds without diagnostics.
- 2026-09-12, final cleanup: scoped whitespace/project checks pass, no README has a diff, `make -C indigo_test test-clean` removed the hardware/fake test binaries, and all owned test processes ended.

## Remaining hardware gaps

This Atik One exposes no guider, so electrical guide direction and pulse timing remain untested. Physical USB removal/replug was not requested during this run; the fake suite covers idle, active-exposure and blocked-readout removal, but this is not physical evidence. Optical image quality, shutter behavior (`libatik` flags did not expose a shutter), cooling accuracy/full warm-up, multiple cameras, other Atik models and non-macOS platforms remain unverified.
