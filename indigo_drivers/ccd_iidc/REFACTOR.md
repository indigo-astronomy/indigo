# CCD IIDC generator migration

Date: 2026-09-15

Baseline revision: `05c12b5fb` with a clean working tree.

## Scope and hardware decision

Migrate `indigo_ccd_iidc` from hand-written lifecycle/hot-plug boilerplate to an `indigo_generator` `.driver` source, retain its libdc1394/IIDC behavior, and support Linux and macOS only. Add deterministic hardware-free fake-libdc1394 integration coverage. Physical validation will be performed at the end with an Atik GP USB IIDC camera; until that run is recorded, hardware validation remains pending and is not implied by fake-SDK results.

Planned Atik GP scenarios: discovery and identity, connect/disconnect/reconnect, supported modes and properties, full-frame and Format7 ROI exposure, shortest/long exposure, abort/reacquire where supported, finite and indefinite streaming stop/restart, gain/gamma/temperature capabilities as exposed, physical unplug/replug idle and during acquisition, driver shutdown/reload, and a fresh exposure after recovery. Initial camera settings will be preserved and restored.

## Current-state audit

- Architecture: one CCD logical device per libdc1394 `(guid, unit)`, maximum 10 devices. The driver owns a process-wide `dc1394_t`, enumerates the SDK after libusb events, stores a `dc1394camera_t *` per device, and uses hand-written attach/change/detach plus libusb hot-plug scaffolding.
- Acquisition: one-shot exposure and streaming use libdc1394 capture queues. The current timer callbacks perform blocking `DC1394_CAPTURE_POLICY_WAIT` dequeue; streaming additionally loops inside one callback. Disconnect/abort stops capture to wake the SDK call.
- Modes: Format7 modes expose each supported color coding with ROI unit alignment; fixed IIDC modes expose legacy fixed geometry. YUV is converted to RGB8; other frame payloads are copied. Binning is intentionally unsupported. Advertised output formats are the seven base CCD formats.
- Properties: inherited CCD properties plus driver-specific use of `CCD_MODE`, `CCD_STREAMING`, `CCD_STREAMING_SETTINGS`, `CCD_GAIN`, `CCD_GAMMA`, `CCD_TEMPERATURE`, and `CCD_IMAGE_FORMAT`. There are no custom properties, so no `X_` naming issue and no property additions/removals are planned.
- Features: shutter/exposure, optional gain and gamma in absolute manual mode, optional read-only temperature, Format7 ROI, fixed modes, still exposure and finite/indefinite streaming. No cooler, guider, wheel, focuser, frame-type-controlled shutter, or device-side binning.
- Lifecycle/ownership: libdc1394 camera objects are created during discovery and freed on removal; the image buffer exists only while connected. A per-device mutex protects capture setup/stop. The process-wide enumeration and device array use another mutex.
- SDK/documentation: repository-bundled libdc1394 sources/headers and its `README`, plus the driver README link to the IIDC/libdc1394 project. No separate manufacturer protocol document is bundled. Atik GP is listed as previously tested hardware.
- Platforms/build: the current README claims platform independence, while this migration intentionally declares only `INDIGO_LINUX || INDIGO_MACOS`. macOS arm64 host build uses a bundled x86_64 libdc1394 via the repository make configuration. Windows will return `INDIGO_UNSUPPORTED_ARCH` from generated metadata fallback. No README change is made because explicit approval to edit README was not requested.
- Integration: `.c`, `.h`, and `_main.c` are registered in Xcode; no `.driver`, refactoring record, fake SDK test, or IIDC hardware test exists. `MIGRATION_STATUS.md` reports API 2, no Windows/generator/async queues/retest, and `0 / 0` tests.
- Existing automated coverage: none for this driver. Generic CCD/framework suites do not substitute for driver-specific SDK and image-contract coverage.

## Baseline evidence

- Host: macOS 26.6.2 (Darwin 25.6.0), arm64; repository `05c12b5fb`.
- `make -C indigo_drivers/ccd_iidc -f ../../Makefile.drv`: PASS (existing artifacts were current; target reports archive, dylib and executable).
- `make ccd_iidc`: not a valid repository target (`No rule to make target`); retained here as baseline command-discovery evidence.
- Driver-specific automated tests: unavailable (no target/source exists), therefore 0 run / 0 passed.
- Linux and physical hardware baseline: not run in this environment.

## Known risks and audit findings

- SDK enumeration failure logs `list->num` and later frees `list` although libdc1394 may not initialize the output pointer.
- Fixed `mode_data[64]` and base `CCD_MODE` storage are populated without capacity validation.
- Several SDK return values are logged but ignored, allowing invalid mode/feature/setup state to appear successful.
- Attach-time SDK calls make discovery attachment fallible without complete rollback.
- Removal matches only GUID, not `(guid, unit)`, and treats an enumeration failure as absence of every device.
- Temperature polling stops permanently after one SDK error.
- Exposure setup failure after shutter programming can leave output properties incompletely finalized.
- Blocking dequeue and the streaming loop monopolize a callback thread/queue and complicate abort, disconnect, removal, and teardown races.
- Frame byte count, geometry, pixel depth/coding, and destination capacity are not validated before copy/conversion.
- Per-device mutex destruction and camera release on normal shutdown are incomplete.

## Atomic plan

1. **DONE — Audit and baseline.** Read repository/driver/test instructions and references, inventory behavior and gaps, build the existing target, decide Atik GP hardware validation, and create this record.
2. **DONE — Generator source.** Reverse extraction was performed only in `/tmp` for guidance. Added version 13 `.driver`, declared `defined(INDIGO_LINUX) || defined(INDIGO_MACOS)`, and regenerated synchronized `.c`, `.h`, and `_main.c`.
3. **DONE — Lifecycle and discovery.** Generator SDK queues now own hot-plug serialization. Discovery keys devices by `(GUID, unit)`, treats failed enumeration as inconclusive, uses generator capacity without a `MAX_DEVICES` override, and releases the camera in `ccd.on_detach` after disconnect processing.
4. **DONE — Camera behavior.** Mode/feature results and geometry are validated; dynamic mode storage replaces the fixed array; exposure and streaming use bounded POLL finalizers; abort/disconnect remain queue-responsive; image sizes/codings/capacity are checked; temperature errors publish ALERT and polling recovers.
5. **DONE — Fake SDK test.** Added 14 deterministic named fake-libdc1394 cases. The fake generates independent synthetic pixels and controls discovery, capabilities, capture readiness, malformed frames, SDK errors and removal order.
6. **DONE — Test/build registration.** Registered the fake suite and sanitizer targets in `indigo_test/Makefile`; registered `.driver`, `REFACTOR.md`, and `test_ccd_iidc_sdk.c` in Xcode. Physical Atik GP execution will use the built driver and client/property harness in the final step; no physical test is part of normal integration.
7. **DONE — Automated verification.** Generator output hashes were identical after regeneration. macOS universal driver build passed. The 14-case normal and ASan/UBSan suites passed. Strict arm64 compile passed after suppressing one generator-owned incompatible-function-cast warning; unsupported-architecture fallback compiled strictly. A native Windows-header compile was unavailable on macOS and is out of declared scope; Linux runtime/build remains unavailable on this host.
8. **DONE — Documentation/status.** Updated the property source mapping and only the ccd_iidc status columns/counts in `MIGRATION_STATUS.md`; preserved its Comment verbatim.
9. **DONE — Final diff audit.** Version is higher (12 to 13), generated sources reproduce, no `MAX_DEVICES` override was introduced, new persistent files are registered, generated architecture fallback is present, and `git diff --check` passes. Build/test artifacts remain only in ignored build locations.
10. **DONE — Atik GP hardware acceptance.** Physical macOS validation on 2026-09-15 passed discovery, identity, mode selection, mono8/mono16 FITS, Format7 ROI, minimum/longer exposure, gain/gamma, temperature, finite/indefinite streaming, abort/reacquire, reconnect, idle and active unplug/replug, fresh exposure after both recoveries, and clean driver unload. Linux remains unverified.

## Coverage matrix

- `Metadata and property contract`: driver metadata, CCD interface, connected property/item inventory, mode count, and deliberately hidden binning.
- `Exposure and image contract`: BUSY-to-OK exposure, RAW publication, deterministic mono8 header/content contract.
- `Format7 ROI and RAW16`: coding selection, ROI unit alignment, SDK geometry, and mono16 frame delivery.
- `YUV and legacy modes`: YUV-to-RGB path, fixed-mode selection/acquisition, and rejection of ROI changes in a legacy mode.
- `Finite and aborted streaming`: exact finite frame count, indefinite stream, stop/abort and finalization.
- `SDK failures and recovery`: capture setup, malformed frame/size, dequeue failure, ALERT cleanup, and reacquisition.
- `Controls and temperature recovery`: gain SDK argument, gamma write error, temperature poll ALERT and subsequent recovery.
- `Busy overlap and disconnect`: mode/exposure overlap guards, pending POLL readout, disconnect cancellation, capture stop and no use after release.
- `Active removal and recovery`: removal during pending exposure, ordered disconnect/detach/release, no use after release, and reattachment.
- `Hotplug identity and inconclusive removal`: duplicate arrival, failed presence enumeration, GUID/unit disappearance, and replug.
- `Multiple devices and capacity`: distinct GUID/unit devices, generator capacity, removal, and recovered capacity.
- `Initialization and registration rollback`: missing SDK context behavior, hot-plug registration failure/retry, and clean shutdown.
- `Connection initialization failure`: required mode discovery failure, transactional cleanup, and successful retry.
- `Rejected shutdown and reconnect`: connected shutdown rejection without losing acquisition, disconnect/reconnect, and fresh operation.

Unsupported/not applicable by design: guider timing, cooling, device-side binning, wheel/focuser/rotator, and SDK configuration persistence (libdc1394 feature persistence is camera-owned and not implemented by this driver). Real FireWire hot-plug cannot be induced by the libusb fake; SDK identity enumeration is covered, while physical FireWire validation is deferred because the selected hardware is USB Atik GP.

## Found defects

- **Source audit, fixed:** enumeration failure dereferenced/freed an indeterminate list. All lists now start NULL, are used only after successful enumeration, and a failure during removal is inconclusive. Regression: `Hotplug identity and inconclusive removal`.
- **Source audit, fixed:** mode/property population used fixed unchecked capacity. Mode metadata and `CCD_MODE` are resized together with allocation/error handling. Regression: `Metadata and property contract` and `Multiple devices and capacity`.
- **Source audit, fixed:** removal used GUID alone and could remove multiple functional units. Identity is now `(GUID, unit)`. Regression: `Hotplug identity and inconclusive removal` and multi-device case.
- **Source audit, fixed:** SDK setup results and frame geometry/size were accepted unchecked, risking false OK or buffer overwrite. Setup is transactional and frames are validated before copy/conversion. Regression: `SDK failures and recovery`.
- **Source audit, fixed:** blocking WAIT dequeue and the streaming loop monopolized the callback. POLL finalizers perform one bounded action per queue turn. Regression: finite/abort, busy/disconnect and active-removal cases.
- **Source audit, fixed:** a temperature read error stopped polling permanently. Polling now publishes ALERT and reschedules. Regression: `Controls and temperature recovery`.
- **Fake-SDK reproduced, fixed:** freeing the camera in `sdk.unplug` occurred before generated detach/disconnect and caused SDK calls through a released handle. Camera release moved to `ccd.on_detach`. Regression: `Active removal and recovery` (initially failed with two post-release calls, now passes with zero).
- **Source audit, fixed:** frame fields were read after returning the frame to the SDK capture queue. Required metadata is copied to local scalars before enqueue. Covered by normal and sanitizer image cases.
- **Hardware reproduced, fixed (version 19 to 20):** a legacy (non-Format7) video mode was selected without programming a framerate, so the camera kept the rate the previous mode had left in the register. Every acquisition in a mode that does not offer that rate timed out. See the 2026-09-22 section. Regression: `Legacy mode framerate` in `indigo_test/integration/test_ccd_iidc_sdk.c` plus the full-mode sweep of the hardware test.
- **Source audit, fixed:** the generated connection path does not call close after a failed low-level open, so partially allocated mode state could survive a failed first connection and leak on removal. `iidc_open()` now performs its own complete rollback. Regression: `Connection initialization failure` and ASan/UBSan suite.

## Verification evidence

- `make -C indigo_drivers/ccd_iidc -f ../../Makefile.drv`: PASS, macOS universal x86_64/arm64 archive, dylib and executable.
- `make -C indigo_test test-ccd-iidc-sdk`: PASS, 14/14 named cases.
- `make -C indigo_test test-ccd-iidc-sdk-sanitize`: PASS, 14/14 with driver and fake compiled under ASan/UBSan (`detect_leaks=0`, static framework/vendor dependencies not sanitizer-instrumented).
- Strict driver compile: PASS on macOS arm64 with `-Wall -Wextra -Werror`, excluding the generator-owned `process_sdk_retry_handler` function-pointer cast warning.
- Strict unsupported-architecture fallback compile: PASS with neither Linux nor macOS macro defined.
- Linux build/runtime: unavailable on this macOS host.
- Physical Atik GP: USB 1e10:2005, SDK model Chameleon CMLN-13S2M, GUID/unit 00b09d0100cd4314-0. Full 1296x964 mono8/mono16 FITS and offset (16,12) 640x480 Format7 acquisition passed with version 15. Exposures 0.00001, 0.02 and 0.06 seconds passed; multi-second exposures exceed the exposed 0.066657-second maximum and are not applicable to this camera configuration. Three-frame stream counted 3→2→1→0; indefinite streaming BUSY→abort→OK and fresh exposure passed. Gain 0→1→0, gamma 1→1.1→1 and temperature readback passed. Both idle and streaming physical unplug/replug recovered identical identity and a fresh exposure. Final disconnect and unload passed with no remaining server.
- Hardware-discovered defects: Format7 position-before-size caused an invalid intermediate full-frame ROI; coding changes after ROI also failed. Mode selection now resets offset/size before coding; capture resets offset before size and requested position. Fake geometry checks and coding-after-ROI regression were added. The final offset-reset extension was build/fake verified; the preceding version-15 ROI and recovery path was physically verified.
- Expected active-unplug behavior: transmission OFF reports Generic failure after USB loss, while capture stop succeeds, transfers are cancelled and helper thread joins. SDK stop occasionally took 0.103–0.107 seconds (queue advisory threshold 0.100 seconds); no hang occurred. This vendor SDK transaction latency is a remaining limitation.

## Final test summary

- Simulated/fake-SDK tests: 32 run, 32 passed (16 normal + the same 16 under ASan/UBSan).
- Hardware tests: 1 run, 1 passed (`make -C indigo_test test-ccd-iidc-hw`, macOS arm64, Atik GP / Chameleon CMLN-13S2M, all 10 modes).
- Hardware acceptance: scenarios above passed on macOS with Atik GP; no Linux or FireWire hardware validation claimed.

## Rejected-change regression coverage (2026-09-18)

`CCD_GAIN` and `CCD_GAMMA` were dispatched with `INDIGO_COPY_VALUES_PROCESS_CHANGE` and written from `number.value`, so a failed `dc1394_feature_set_absolute_value()` left the refused value in the property permanently, with no previous value kept anywhere. Both properties now use `preserve_values = true`, are written from `number.target`, restore `target` from `value` on failure and commit `value` on success; `iidc_setup_feature()` seeds `value` from the camera at connect, so the restored value is device truth. The `CCD_MODE` and `CCD_FRAME` busy guards returned silently without publishing anything and are now declared with the generator's `reject_change` block, so the client gets ALERT, a message and the actual values.

Covered by `Rejected change keeps values` in `indigo_test/integration/test_ccd_iidc_sdk.c`: injected `gain` and `gamma` SDK failures and then an exposure in progress leave the property in ALERT with unchanged value and target, and gain is accepted again afterwards.

```sh
cd indigo_test && ./build/integration/test_ccd_iidc_sdk "Rejected change"
```

## Atik GP hardware regression test (2026-09-18)

`indigo_test/hardware/test_ccd_iidc_hw.c` and `make -C indigo_test test-ccd-iidc-hw` turn the manual Atik GP acceptance run into a repeatable test: discovery and identity, connect, every advertised `CCD_MODE` with one exposure each, `CCD_GAIN` and `CCD_GAMMA`, finite and indefinite streaming with abort, refused geometry changes during acquisition, disconnect/reconnect, driver shutdown/reload and a fresh exposure, plus a check that no frame violated the RAW header contract. `HW_HOTPLUG=1` adds the physical unplug/replug section; it was not exercised in this run.

The accepted control change is checked the same way as the refused one: after `CCD_GAIN` or `CCD_GAMMA` reaches OK, both `value` and `target` must hold the requested number, and the original setting is restored with the same check. The requested number is an advertised limit rather than the midpoint, because the camera can already sit at the midpoint and the check would then compare a value with itself. The refused path of these two properties still needs an injected `dc1394_feature_set_absolute_value()` failure and stays in `test_ccd_iidc_sdk.c`; the `CCD_MODE` and `CCD_FRAME` busy guards, on the other hand, are reachable on hardware and are covered here by requesting a geometry change during indefinite streaming and requiring ALERT with every value and target unchanged. The in-process client sees the driver-side property directly, so the `do_update` item marking of `reject_change` is not observable here and remains covered at protocol level.

Found and fixed (version 16 to 17): aborting an acquisition published `CCD_ABORT_EXPOSURE` in ALERT. The handler settled `CCD_STREAMING` to OK before calling `indigo_ccd_abort_exposure_cleanup()`, so the cleanup saw neither an exposure nor a stream in progress and took its "nothing was running" branch. The handler now only finalizes the video stream and leaves both states to the cleanup, as `ccd_asi` and `ccd_playerone` already do. A consequence of the delegation is that aborting a *finite* stream now reports `CCD_STREAMING` ALERT instead of OK, which is the framework contract for an incomplete stream. Regression: the abort step of the hardware test, which fails when the old handler body is put back.

`MODE_3` (`MONO 16 1280x960`, legacy fixed mode) never delivered a frame on this camera and was recorded here as a camera/SDK limitation. **That conclusion was wrong**; see the 2026-09-22 section below, which finds the driver defect behind it and fixes it. `HW_SKIP_MODES` stays available for a camera that really cannot run a mode:

```sh
make -C indigo_test test-ccd-iidc-hw HW_SKIP_MODES=MODE_3
```

Result on 2026-09-18 with Atik GP (Chameleon CMLN-13S2M, USB 1e10:2005): passed, 9 of 10 modes exercised, 0 invalid frames. `make -C indigo_test test-ccd-iidc-sdk` and `test-ccd-iidc-sdk-sanitize` pass unchanged with version 17.

## Legacy-mode framerate defect (2026-09-22)

Non-interactive hardware run on macOS arm64 against the Atik GP (Chameleon CMLN-13S2M, USB `1e10:2005`), version 19. `MODE_3` (`MONO 16 1280x960`, legacy) failed with `CCD_EXPOSURE` ALERT and "Exposure timed out or readout failed", exactly as recorded on 2026-09-18.

**Root cause.** `iidc_select_mode()` called `dc1394_video_set_mode()` and, for Format7, programmed position, size and coding, but for a legacy mode it programmed nothing else. A legacy IIDC mode carries its own framerate list, and the camera keeps whatever the previously selected mode left in the framerate register. A standalone libdc1394 probe reports the Chameleon's lists as

| driver mode | dc1394 mode | framerates |
| --- | --- | --- |
| `MODE_0` `MONO 8 640x480` | 69 | 1.875, 3.75, 7.5, 15, 30 |
| `MODE_1` `MONO 16 640x480` | 70 | 1.875, 3.75, 7.5, 15, 30 |
| `MODE_2` `MONO 8 1280x960` | 81 | 1.875, 3.75, 7.5, 15 |
| `MODE_3` `MONO 16 1280x960` | 85 | 1.875, 3.75, 7.5 |

and confirms the register held 15 while mode 85 was selected - a combination the mode does not offer, so the camera never started transmitting and the 30-second readout deadline expired. The mode sweep reaches `MODE_3` straight after `MODE_2`, which is why the failure looked mode-specific. The 2026-09-18 note that a standalone program reproduced it "with an explicitly selected supported one (7.5 fps)" does not hold: with a supported rate actually programmed, the mode delivers a full 2457612-byte frame.

**Fix (version 20).** `iidc_select_framerate()` runs for every non-Format7 mode after `dc1394_video_set_mode()`. It keeps the current rate when the new mode's list contains it and otherwise programs the fastest rate the mode does support; a failure of any of the three transactions fails mode selection, so `CCD_MODE` reaches ALERT instead of leaving the camera mute. Format7 modes have no framerate and are untouched.

**Consequence on the exposure range.** The framerate bounds the shutter: this camera reports `max_shutter = 245.33 / framerate`, so the advertised `CCD_EXPOSURE` maximum moved from the 0.0666565 s recorded in the older runs to 16.3555 s, and it changes with the selected mode. That is not new behaviour - `CCD_MODE` already re-reads the shutter range through `iidc_setup_feature()` after every mode change - but before the fix the range depended on whatever rate the camera happened to hold, and it is now reproducible across connects.

**Coverage.** `Legacy mode framerate` in `indigo_test/integration/test_ccd_iidc_sdk.c` drives the fake camera from a rate the legacy mode does not offer, requires the driver to program 7.5, requires a supported rate to survive untouched, and requires a failing `dc1394_video_get_supported_framerates()` or `dc1394_video_set_framerate()` to refuse the mode change with ALERT and no rate written. The fake gained `dc1394_video_get_supported_framerates`, `dc1394_video_get_framerate` and `dc1394_video_set_framerate`.

**Stale expectation repaired in the same run.** `Hotplug identity and inconclusive removal` failed before any change of this session. Commit `0057e7302` made `sdk.unplug_match` confirm a removal libusb could not identify rather than veto one it reported, so a failing `dc1394_camera_enumerate()` no longer keeps a device whose own libusb device left. The case now sends the inconclusive event with a foreign libusb token, which is the path the block still serves, and additionally requires that a camera the enumeration keeps listing does not survive its own `DEVICE_LEFT`.

**Results.** `make -C indigo_test test-ccd-iidc-sdk` 16/16, `test-ccd-iidc-sdk-sanitize` 16/16, `make -C indigo_test test-ccd-iidc-hw` 1/1 with all 10 modes exercised and 0 invalid frames, repeated three times with identical output. Hot-plug was out of scope for this camera: it is attached directly to the host, not to the Pegasus UPB hub that the other cameras of the session use for unattended unplug.
