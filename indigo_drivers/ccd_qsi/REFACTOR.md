# INDIGO 3.0 refactoring record for `ccd_qsi`

## Goal and scope

Migrate the hand-written Quantum Scientific Imaging camera driver to `indigo_generator`, make the generated `.driver` definition the source of truth, move all device work onto INDIGO handler queues, and add comprehensive hardware-free integration coverage through a deterministic fake `qsiapi` SDK. Preserve the existing CCD and internal filter-wheel behavior unless a documented defect requires a tested correction.

The supported implementation and validation scope is macOS (x86_64 **and arm64**) and Linux (x86, x64, ARM v6+, ARM64). Windows is explicitly out of scope: no QSI Windows SDK is bundled. Apple Silicon support was investigated and then enabled as part of this work; see [Apple Silicon support](#apple-silicon-support).

## References read

- Root `AGENTS.md` and `indigo_drivers/AGENTS.override.md`.
- `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, including the `sdk` hot-plug, `attach_if`, `name_value`, `reject_change`, `_finalizer` and `cpp = true` sections.
- `indigo_test/AGENTS.md` and the CCD and wheel acceptance standards in `indigo_test/DRIVER_TESTING_RULES.md`.
- Driver source `indigo_ccd_qsi.cpp`, public header, executable main, `Makefile.inc`, `indigo_ccd_qsi.rules`, driver `README.md`.
- Bundled SDK: `bin_externals/qsiapi-7.6.0/build/include/qsiapi.h`, `QSIError.h`, `lib/*` sources, `configure.ac`, `lib/Makefile.am`, `doc/QSI Linux API Reference Manual.pdf` (present, not machine-readable in this environment).
- `MIGRATION_STATUS.md`, `indigo_docs/PROPERTIES.md`.
- Reference generated SDK camera drivers used as implementation models: `indigo_drivers/ccd_mi/indigo_ccd_mi.driver` (CCD + internal wheel, SDK hot-plug, shared open reference counting) and `indigo_drivers/ccd_qhy/indigo_ccd_qhy.driver` (`cpp = true`, C++ vendor SDK, `supported_architecture`).
- Reference fake-SDK suites: `indigo_test/integration/test_ccd_mi_sdk.c` and `indigo_test/integration/test_ccd_qhy_sdk.cpp`.

## Baseline audit

### Repository and environment

- Baseline revision: `1e2bedee911f75541db73a4d8503c6d953902a60` (branch `refactoring`), clean working tree apart from pre-existing build products in the driver directory.
- Host: macOS 26.6.2 (Darwin 25.6.0), Apple Silicon `arm64`; Apple clang 21.0.0.
- Current driver version: `0x0200000D` (API 2, revision 13).
- Bundled SDK: `qsiapi-7.6.0`, prebuilt static archives for Linux `arm`, `arm64`, `x64`, `x86` and macOS. **The macOS archive is `x86_64` only** (`lipo -info` → `Non-fat file: ... is architecture: x86_64`).
- The SDK links against FTDI D2XX (`-lftd2xx`) and `libusb-1.0`. At baseline, `build/lib/libftd2xx.a` was installed by `indigo_libs/Makefile` from `bin_externals/D2XX1.4.4.dmg`, which was also **`x86_64` only** on macOS. Both vendor archives were replaced during this work, see [Apple Silicon support](#apple-silicon-support).

### Architecture and lifecycle

- The current implementation is a 1034-line hand-written C++ driver (`indigo_ccd_qsi.cpp`) with a hand-written public header and executable main; no `.driver` input exists.
- The whole implementation is wrapped in `#if !(defined(__APPLE__) && defined(__arm64__))`; the Apple Silicon build emits a stub entry point returning `INDIGO_UNSUPPORTED_ARCH`.
- The vendor SDK exposes a **single global `QSICamera cam` object**. The driver therefore declares one file-scope `static QSICamera cam` and serializes access with one file-scope `pthread_mutex_t indigo_device_enumeration_mutex`. Only one camera and one filter wheel can be connected at a time; this is a documented SDK limitation (driver `README.md`).
- USB hot-plug registers two libusb callbacks for vendor `0x0403`, products `0xEB48` and `0xEB49`. Arrival and removal both schedule a timer that sleeps one second and then re-enumerates with `cam.get_AvailableCameras()`.
- One physical camera yields one CCD logical device. The filter-wheel logical device is **created inside the CCD connection handler** with `malloc` + `indigo_attach_device()` after `get_HasFilterWheel()` reports a wheel, and destroyed on CCD disconnect. Both logical devices share one `qsi_private_data`.
- `devices[QSICamera::MAXCAMERAS]` is a 128-entry registry, so `MAX_DEVICES` is effectively 128 in the original.
- CCD temperature and cooler power use a recurring 5 s timer. Exposure completion uses a one-shot timer armed for the requested duration whose callback then enters a **blocking `get_ImageReady()` polling loop with `indigo_usleep(5000)`**, with no timeout. Filter-wheel movement enters a **blocking `get_Position()` loop with `indigo_usleep(100000)`**, also with no timeout.

### Public properties and capabilities

- Driver-specific custom properties, all switches in `AT_MOST_ONE` rule:
  - `QSI_READOUT_SPEED` (`HIGH_QUALITY`, `FAST_READOUT`), group `CCD_ADVANCED_GROUP`.
  - `QSI_ANTI_BLOOM` (`NORMAL`, `HIGH`), group `CCD_ADVANCED_GROUP`.
  - `QSI_PRE_EXPOSURE_FLUSH` (`NONE`, `MODEST`, `NORMAL`, `AGGRESSIVE`, `VERY_AGGRESSIVE`), group `CCD_ADVANCED_GROUP`.
  - `QSI_FAN_MODE` (`OFF`, `QUIET`, `FULL_SPEED`), group `CCD_COOLER_GROUP`.
  These names violate the repository rule that driver-specific property names must start with `X_`. They are renamed during migration; see [Found defects](#found-defects), defect D6.
- All four custom properties are saved by an explicit `CONFIG_SAVE` branch and are hidden when the SDK reports an unexpected enum value.
- Inherited CCD surface: `CCD_INFO`, `CCD_FRAME`, `CCD_BIN`, `CCD_MODE` (equal bins only, doubling when `PowerOfTwoBinning`), `CCD_EXPOSURE` (min/max from `get_MinExposureTime`/`get_MaxExposureTime`), `CCD_ABORT_EXPOSURE`, `CCD_FRAME_TYPE`, `CCD_GAIN` (only when `CanSetGain`; 0..2 mapping to `CameraGain`), `CCD_COOLER`, `CCD_COOLER_POWER` (only when `CanGetCoolerPower`), `CCD_TEMPERATURE` (only when `CanSetCCDTemperature`; range -60..60).
- Inherited wheel surface: `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET` sized from `get_FilterCount()`; public slot is SDK position + 1.
- `INFO_PROPERTY->count = 8`, `INFO_DEVICE_SERIAL_NUM` from the enumerated serial, `INFO_DEVICE_MODEL` set to `"QSI <model>"` after connect.
- `indigo_docs/PROPERTIES.md` has **no** `ccd_qsi` section; one must be added.

### SDK contract and error handling

- `qsiapi.h` methods return `int` (`QSI_OK` = 0, otherwise a `QSI_*` code from `QSIError.h`). The API can additionally be switched to structured exceptions with `put_UseStructuredExceptions(bool)`.
- **Structured exceptions are the SDK default.** `CCDCamera.cpp:94` sets `m_bStructuredExceptions = true` in the constructor, and the `Error(x, y, z)` macro in `lib/wincompat.h` throws `std::runtime_error("0x<code>:<text>")` when that flag is set and returns the code otherwise. The original driver's `catch (std::runtime_error err)` blocks therefore *do* receive every SDK error under default settings.
- The driver never checks an `int` return code. That is latent rather than live: it only matters if the exception mode is ever switched off. The migrated driver handles both channels (see defect D1).
- Discovery: `get_AvailableCameras(serial[], desc[], count)` fills caller arrays sized `QSICamera::MAXCAMERAS`.
- Connect sequence in the original, in order: `get_Connected`, (bail out if already connected), `put_SelectCamera(serial)`, `put_IsMainCamera(true)`, `put_Connected(true)`, `get_ModelNumber`, `get_CameraXSize`, `get_CameraYSize`, `get_PixelSizeX`, `get_PixelSizeY`, allocate blob buffer, `get_CanSetCCDTemperature`, `get_HasShutter`, `get_MinExposureTime`, `get_MaxExposureTime`, `get_HasFilterWheel`, `get_FilterCount` (if wheel), `get_CanSetGain`, `get_CameraGain` (if settable), `get_MaxBinX`, `get_MaxBinY`, `get_PowerOfTwoBinning`, `get_FanMode`, `get_CoolerOn` (if `CanSetCCDTemperature`), `get_CanGetCoolerPower`, `get_ReadoutSpeed`, `get_AntiBlooming`, `get_PreExposureFlush`.
- Exposure sequence: `put_StartX`, `put_StartY`, `put_NumX`, `put_NumY`, `put_BinX`, `put_BinY`, `StartExposure(duration, light)`, then `get_ImageReady` polling, `get_ImageArraySize`, `get_ImageArray(unsigned short *)`.
- Abort sequence: `get_CanAbortExposure`, `AbortExposure()`.
- Wheel: `get_Position(short *)` returns `-1` while moving; `put_Position(short)` starts a move.
- This call order and manner is treated as the compatibility contract and is preserved by the migration; deviations are listed under [Intentional behavioral differences](#intentional-behavioral-differences).

### Existing test coverage

- There is no `test_ccd_qsi_sdk` source, build target, fake SDK, simulator or opt-in hardware test.
- Baseline relevant automated cases: 0 hardware-free, 0 hardware.
- Baseline build command: `make -B -C indigo_drivers/ccd_qsi -f ../../Makefile.drv all` (run as `rm -f *.o && make -f ../../Makefile.drv`).
- Baseline result: **PASS**. `indigo_ccd_qsi.a`, `indigo_ccd_qsi.dylib` and `indigo_ccd_qsi` were produced as `x86_64`/`arm64` fat binaries. No compiler diagnostics. The linker emitted pre-existing warnings that every object of `bin_externals/qsiapi-7.6.0/build/lib/macOS/libqsiapi.a` and of `build/lib/libftd2xx.a` is `x86_64` and is ignored for the `arm64` slice; the `arm64` slice contains only the unsupported-architecture stub, so the link still succeeds.
- No baseline driver-specific tests were available to run. The mandatory characterization suite is therefore built first against the **original** driver (plan step 3) before any production change.

### Initial risks and candidate defects from source audit

1. No SDK return code is ever checked; error handling relies entirely on the exception channel, which is the SDK default but is not guaranteed (D1).
2. Unbounded blocking poll loops for image readiness and wheel position run on the device timer, holding the queue and offering no timeout or abort (D2).
3. `PRIVATE_DATA->buffer` is freed only on a successful `put_Connected(false)`; a disconnect failure leaks the blob buffer (D3).
4. `process_plug_event` allocates a `qsi_private_data` with `indigo_safe_malloc` before looking for a free registry slot and never frees it if none exists (D4).
5. No exposure countdown is published: `CCD_EXPOSURE_ITEM->number.value` jumps from the requested duration to 0. The shared `indigo_ccd_exposure_setup()` countdown is not used (D5).
6. Custom property names do not use the mandatory `X_` prefix (D6).
7. `wheel_slot_callback` re-validates `WHEEL_SLOT_ITEM->number.value` against `number.max`, duplicating framework range validation that `AGENTS.md` forbids in drivers (D7).
8. The `CONFIG_SAVE` branch is hand-written instead of using the generator `persistent` attribute (D8).
9. `wheel_connect_callback` skips the base-class `CONNECTION` finalization on failure (D9).
10. `process_unplug_event` leaks the blob buffer of a connected camera (D10).

## Hardware-test result

**Decision: no hardware testing.** No QSI camera (500/600/RS series) is available in this environment, and the macOS host is Apple Silicon, where the current driver cannot even run. No hardware validation is claimed anywhere in this record. Hardware acceptance per `indigo_test/DRIVER_TESTING_RULES.md` remains deferred and is listed in [Deferred work](#deferred-work).

## Apple Silicon support

At baseline the driver excluded Apple Silicon with `#if !(defined(__APPLE__) && defined(__arm64__))`. The investigation showed the restriction came from the two vendor binaries, not from QSI code, and both were replaced, so the restriction is now gone.

### What the investigation found

1. **The bundled prebuilt macOS QSI archive was Intel-only.** `lipo -info bin_externals/qsiapi-7.6.0/build/lib/macOS/libqsiapi.a` → `Non-fat file: ... is architecture: x86_64`.
2. **The QSI SDK sources in the tree compile for `arm64` unchanged.** All 21 translation units of `bin_externals/qsiapi-7.6.0/lib` build with `clang++ -arch arm64`. The only missing input was the autoconf-generated `config.h`, which supplies nothing but `PACKAGE_VERSION`. There is no Intel-specific code, inline assembly or endianness assumption.
3. **The only external dependency was FTDI D2XX.** `nm -u` on the `arm64` archive showed the sole unresolved vendor symbols were `FT_Close`, `FT_CreateDeviceInfoList`, `FT_GetDeviceInfoList`, `FT_GetQueueStatus`, `FT_GetStatus`, `FT_ListDevices`, `FT_OpenEx`, `FT_Purge`, `FT_Read`, `FT_ResetDevice`, `FT_SetBitMode`, `FT_SetChars`, `FT_SetFlowControl`, `FT_SetLatencyTimer`, `FT_SetTimeouts`, `FT_SetVIDPID` and `FT_Write`. The bundled `bin_externals/D2XX1.4.4.dmg` shipped `libftd2xx.a` as `x86_64` only.

FTDI has shipped the macOS D2XX library as a universal binary since 1.4.24; `libftdi1` (open source, libusb based, `--with-ftd=ftdi1` is already supported by the SDK `configure.ac`) would have been the alternative route.

### What was changed

- **`bin_externals/D2XX1.4.4.dmg` was replaced by `bin_externals/D2XX1.4.35.dmg`.** The 1.4.35 libraries are universal: `lipo -info` reports `x86_64 arm64 arm64e` for `libftd2xx.a` and both dylibs.
- **`indigo_libs/Makefile` was updated for the new disk image.** The 1.4.35 layout differs from 1.4.4 (`<volume>/release/…` with the libraries under `release/build/` instead of `<volume>/D2XX/…`), so the rule now mounts on an explicit `/tmp/indigo_d2xx` mountpoint instead of relying on the default volume name.
- **The libusb-stripping step became architecture aware.** The vendor archive embeds its own libusb and INDIGO links its own `libusb-1.0`, so those members are deleted. That cannot be done on a universal archive in one `ar` call, so each slice is now thinned with `lipo -thin`, stripped, re-indexed with `ranlib` and recombined with `lipo -create`. The member list also had to be split per platform: 1.4.35 carries libusb 1.0.29, which renamed `poll_posix.o` to `events_posix.o` and added `strerror.o`, and macOS `ar` fails on a member that is absent while GNU `ar` only warns. The Linux list and the Linux 1.4.6 archives are unchanged.
- **`bin_externals/qsiapi-7.6.0/build/lib/macOS/libqsiapi.a` is now universal (`x86_64 arm64`).** The original vendor `x86_64` slice is kept byte for byte; only an `arm64` slice was added, built from the bundled SDK sources so existing Intel behavior cannot regress.
- **`supported_architecture` was removed from `indigo_ccd_qsi.driver`** and the driver version was raised to 15. Every platform the repository builds for now has a matching QSI archive, so no restriction remains.
- **`indigo_test/Makefile` no longer pins the QSI test to `-arch x86_64`**; it builds the normal universal test binary.

### Reproducing the arm64 slice

```sh
cd indigo_drivers/ccd_qsi/bin_externals/qsiapi-7.6.0/lib
printf '#define PACKAGE_VERSION 7.6.0\n' > /tmp/qsi_cfg/config.h
# every .cpp of libqsiapi_la_SOURCES, plus ConvertUTF.c with clang
clang++ -arch arm64 -mmacosx-version-min=10.10 -fPIC -O3 -fpermissive -w \
        -DUSELIBFTD2XX -DNO_CYUSB -std=gnu++11 \
        -I/tmp/qsi_cfg -I<repo>/build/include -I. -c <source>.cpp
ar rcs libqsiapi_arm64.a *.o && ranlib libqsiapi_arm64.a
lipo -create <original x86_64 libqsiapi.a> libqsiapi_arm64.a -output libqsiapi.a
```

`config.h` is the autoconf output and only defines `PACKAGE_VERSION`. `-DUSELIBFTD2XX -DNO_CYUSB` matches the SDK default (`--with-ftd=ftd2xx`, Cypress support disabled). `<repo>/build/include` must precede `-I.` so that `#include <ftd2xx.h>` in `HostIO_USB.h` resolves to the installed 1.4.35 header rather than the stale 2011 copy in the SDK `lib` directory.

### Verification

- The `arm64` slice exports exactly the same QSI API surface as the vendor `x86_64` slice: 287 `QSICamera` / `CCCDCamera` / `QSIException` symbols, `diff` clean. The only symbol-set differences between the two slices are libc++ template instantiations, which is expected from a newer toolchain.
- `make -f ../../Makefile.drv` now produces `x86_64`/`arm64` outputs with **no** "ignoring file … required architecture arm64" linker warnings, and the generated source no longer contains `INDIGO_UNSUPPORTED_ARCH`.
- The `arm64` slice of `indigo_ccd_qsi.a` contains 54 `QSICamera` references, so it holds the real implementation and not the former stub, and the linked dylib has no unresolved `FT_*` symbols.
- The full test suite passes on both slices and under sanitizers on `arm64`; see [Final test summary](#final-test-summary).

### Not verified

No QSI camera was available, so Apple Silicon support is proven at the build, link and fake-SDK level only. **No hardware validation of the `arm64` build was performed.** The `arm64e` slice that D2XX 1.4.35 additionally provides is unused, because INDIGO macOS binaries are built for `x86_64` and `arm64` only.

## Atomic plan

Status values are `PENDING`, `IN PROGRESS`, `DONE` or `BLOCKED`. A step is only marked `DONE` after its stated verification has actually been executed and passed, and the actual result is recorded in the same edit.

| # | Step | Verification | State | Result |
| - | ---- | ------------ | ----- | ------ |
| 1 | Read all applicable instructions, driver sources, SDK headers/sources, build files and reference drivers. | Audit recorded below. | DONE | Recorded in this file. |
| 2 | Create this `REFACTOR.md` with audit, baseline, hardware decision and plan before any production change. | File exists with all mandatory sections. | DONE | This file. |
| 3 | Record the baseline build of the unmodified driver. | `make` succeeds; output archived here. | DONE | PASS, see [Existing test coverage](#existing-test-coverage). |
| 4 | Determine Apple Silicon feasibility for the bundled SDK. | Compile the SDK sources for `arm64`; list unresolved vendor symbols. | DONE | See [Apple Silicon support](#apple-silicon-support). |
| 5 | Build a deterministic fake `qsiapi` SDK and the CCD/wheel characterization suite; register the target in `indigo_test/Makefile`. | Suite compiles and links against the **original** hand-written driver. | DONE | `indigo_test/integration/qsi_fake_sdk.h`, `qsi_fake_sdk.cpp` and `integration/test_ccd_qsi_sdk.c`. The fake implements the `QSICamera` class of the real vendor header, so the driver compiles unmodified against `qsiapi.h`. Built for `x86_64` because the original driver compiles to the unsupported-architecture stub on `arm64`. |
| 6 | Run the characterization suite against the original driver; record every expected baseline failure as a dedicated defect reproducer. | All preservation cases pass; each known-defect reproducer is named here. | DONE | **46 of 48 cases pass** against the unmodified driver (built from the `git show HEAD:` copy of the pre-migration sources). The two failures are the dedicated reproducers for newly confirmed defects D11 and D12 and are recorded as expected baseline failures, not weakened. |
| 7 | Capture the normalized original-driver reference trace of ordered SDK interactions and property transitions. | Trace file written and stable across two runs. | DONE | `indigo_test/fixtures/ccd_qsi/original_reference_trace.txt` (62 lines), captured with `test_ccd_qsi_sdk --trace`. Two consecutive captures are byte-identical. Recurring temperature polling collapses into one `[periodic temperature poll]` marker so the trace is host independent. |
| 8 | Write `indigo_ccd_qsi.driver` and generate `.cpp`/`.h`/`_main.c`; keep `cpp = true` and the architecture restriction; remove the superseded hand-written sources. | Generator runs clean; driver builds. | DONE | `indigo_ccd_qsi.driver` is the source of truth. `indigo_ccd_qsi_main.cpp` was replaced by the generated `indigo_ccd_qsi_main.c`. `make -f ../../Makefile.drv` produces the archive, dylib and executable as `x86_64`/`arm64`, with no compiler diagnostics and only the pre-existing linker warnings that the Intel-only vendor archives are ignored for the `arm64` slice. |
| 9 | Re-run the characterization suite and the trace comparison against the migrated driver. | Suite passes; every trace difference justified in [Intentional behavioral differences](#intentional-behavioral-differences). | DONE | Every characterization case passes, including the two scenarios that were expected baseline failures. The D11 case passes unchanged; the D12 case was rewritten as `qsi_recovers_from_disconnect_failure` because the fix changes the correct outcome from "report ALERT" to "complete the disconnect and stay usable", and `qsi_rejects_target_change_while_cooling` was added for the temperature BUSY guard, taking the suite to 49 at this point. The trace comparison is reproduced in [Reference trace comparison](#reference-trace-comparison); the only differences are the discovery probe and the removed re-enumeration, both planned. |
| 10 | Turn each baseline defect reproducer into a passing regression test and extend the suite to the remaining acceptance areas. | Suite passes with the reproducers enabled. | DONE | Suite grown to **60 cases**, all passing. |
| 11 | Update `MIGRATION_STATUS.md`, `indigo_docs/PROPERTIES.md` and `indigo.xcodeproj/project.pbxproj`. | Files consistent with the implemented driver. | DONE | `ccd_qsi` row set to API 3, generator yes, async queues yes, retested Sim, 60 / 0 automated tests, Comment column untouched. A new `ccd_qsi` section was added to the properties reference. The `.driver`, `REFACTOR.md`, both fixtures and the three test sources were added to the Xcode project (`plutil -lint` passes). |
| 12 | Final verification: regeneration reproducibility, strict build, sanitizer run, diff audit, artifact cleanup. | All recorded in [Final test summary](#final-test-summary). | DONE | Re-running the generator leaves the checked-in output byte-identical. ASan + UBSan run is clean. See [Final test summary](#final-test-summary). |
| 13 | Enable Apple Silicon by replacing both Intel-only vendor binaries, then drop the architecture restriction. | Driver builds for `arm64` without the stub; suite passes on both slices. | DONE | See [Apple Silicon support](#apple-silicon-support). Driver version raised to 15. |

## Found defects

Candidates from the source audit are listed here with the evidence class stated explicitly. A candidate is promoted to a confirmed defect only once a reproducer actually fails against the original driver; the fix and regression-test columns are filled in when that work is done.

| ID | Evidence class | Impact | Root cause | Fix | Regression test |
| -- | -------------- | ------ | ---------- | --- | --------------- |
| D1 | Source audit, latent only | Every SDK error is delivered as an exception under default settings, so this is **not** a live failure. If `put_UseStructuredExceptions(false)` were ever in effect, or a future SDK build defaulted differently, every failed `StartExposure`/`put_CoolerOn`/`put_Position`/control write would be published as `OK` with a value the camera never accepted. | No `int` return code is ever inspected; error handling depends entirely on the exception channel. | Every SDK call goes through the `QSI_CALL` wrapper, which catches `std::runtime_error` **and** checks the return code, records the message and returns `bool`. | `qsi_reports_control_write_failures`, `qsi_reports_exposure_start_failure`, `qsi_reports_cooler_and_temperature_failures`, `qsi_reports_wheel_move_failure` |
| D2 | Source audit, reproducer pending | An exposure whose image never becomes ready, or a wheel that never leaves position `-1`, blocks the device timer indefinitely; abort and disconnect cannot proceed. `indigo_cancel_timer_sync()` on the disconnect path waits on a callback that can never return. | `exposure_timer_callback` and `wheel_slot_callback` busy-wait in `while (!ready) indigo_usleep(...)` loops with no deadline. | Both waits are now bounded `_finalizer` handlers scheduled on the queue against an explicit deadline (`QSI_READOUT_TIMEOUT`, `QSI_WHEEL_TIMEOUT`), publishing `ALERT` on expiry. | `qsi_times_out_stuck_readout`, `qsi_times_out_stuck_wheel`, `qsi_disconnects_while_readout_pending`, `qsi_aborts_exposure_and_reacquires` |
| D3 | Source audit | Blob buffer is leaked whenever `put_Connected(false)` fails. | `free(PRIVATE_DATA->buffer)` sits inside the `try` block after the failing call. | `qsi_close()` releases the SDK connection and the buffer unconditionally and is the single owner of the close path. | `qsi_recovers_from_disconnect_failure` (plus the clean ASan run) |
| D4 | Source audit | A `qsi_private_data` allocation leaks per arrival once the 128-slot registry is full. | `process_plug_event` allocates private data before searching for a free slot and never frees it when none is found. | The generator owns allocation and slot assignment; `sdk.plug` checks the free-slot count **before** accepting a device and rejects the arrival otherwise. | Covered by the generated allocation path; exercised by `qsi_handles_burst_arrivals` and `qsi_accepts_both_product_ids` (plus the clean ASan run) |
| D5 | Source audit, reproducer pending | No exposure progress is published; `CCD_EXPOSURE.EXPOSURE` jumps from the requested duration to 0. | The driver arms a single timer for the whole duration and does not use the shared CCD countdown `indigo_ccd_exposure_setup()`. | `indigo_ccd_exposure_setup(device)` is called when the exposure starts, so the framework publishes the shared countdown. | `qsi_publishes_exposure_countdown` |
| D6 | Source audit against `AGENTS.md` | Driver-specific properties use a `QSI_` prefix instead of the mandatory `X_`. | Predates the naming rule. | Renamed to `X_QSI_READOUT_SPEED`, `X_QSI_ANTI_BLOOM`, `X_QSI_PRE_EXPOSURE_FLUSH` and `X_QSI_FAN_MODE`; item names are unchanged. | `qsi_publishes_custom_properties` |
| D7 | Source audit against `AGENTS.md` | Driver duplicates framework numeric range validation for `WHEEL_SLOT`. | Hand-written guard in `wheel_slot_callback`. | Removed; the framework validates the range and the driver validates only the SDK reply. | `qsi_moves_wheel_to_first_last_and_intermediate_slots`, `qsi_reports_wheel_move_failure` |
| D8 | Source audit | Custom-property persistence is a hand-written `CONFIG_SAVE` branch rather than the generator `persistent` attribute. | Predates the generator. | The four custom properties declare `persistent = true`, so the generator owns the `CONFIG` branch. | Not separately tested; see [Deferred work](#deferred-work) for the justification. |
| D9 | Source audit, reproducer pending | When the filter-wheel connection fails, `wheel_connect_callback` returns without calling `indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY)`, so the base class never finalizes `CONNECTION` and the device is left with the connected switch set while the state is `ALERT`. | Early `return` inside the `catch` block. | The generator owns the whole `CONNECTION` lifecycle, including the failure path and the final base-class call. | `qsi_wheel_connect_fails_without_filter_wheel` |
| D10 | Source audit | `process_unplug_event` frees `device->private_data` but never frees `PRIVATE_DATA->buffer`, so unplugging a connected camera leaks the full blob buffer. `remove_all_devices()` does free it, so the two teardown paths disagree. | Buffer ownership is split between the connection handler and two different teardown paths. | `qsi_close()` is the single buffer owner and the generator owns detach/free for removal and shutdown alike. | `qsi_removes_device_with_exposure_pending` (plus the clean ASan run) |
| D11 | **Reproduced** against the original driver | A connection that fails *after* `put_Connected(true)` (for example a failing `get_CameraXSize`) reports `ALERT` but never disconnects the camera in the SDK. The camera is then permanently unusable: every later connect sees `get_Connected() == true` and is refused with "already connected", until the driver is reloaded. | The `catch` block in `ccd_connect_callback` performs no rollback of the SDK connection or of the already allocated blob buffer. | `qsi_open()` is transactional: any failure after `put_Connected(true)` calls `put_Connected(false)` and frees the buffer before returning false. | `qsi_reports_geometry_read_failure` (was an expected baseline failure, now passes) |
| D12 | **Reproduced** against the original driver | A failing `put_Connected(false)` reports `ALERT` while the connection switch is already `DISCONNECTED`, so the framework ignores every further disconnect request. INDIGO believes the device is disconnected while the SDK still holds the camera, and there is no recovery path short of a driver reload. | The disconnect `catch` block leaves the public switch and the SDK state inconsistent and owns no retry. | `qsi_close()` always releases the driver-owned resources and completes the disconnect, and `qsi_open()` adopts an SDK session that a failed disconnect left behind, so the device recovers without a driver reload. | `qsi_recovers_from_disconnect_failure` (replaces the expected baseline failure) |

## Intentional behavioral differences

Every difference below was confirmed against the reference-trace comparison and the characterization suite in plan step 9.

1. **Filter-wheel logical device is attached at discovery, not at CCD connect.** The original `malloc`ed and attached the wheel inside the CCD connection handler. The generator owns logical-device allocation and attachment at plug time through `attach_if`, so the wheel presence must be known during discovery. `sdk.plug` therefore probes the camera (`put_SelectCamera` → `put_Connected(true)` → `get_HasFilterWheel` → `get_FilterCount` → `put_Connected(false)`) exactly like `ccd_qhy` probes with `OpenQHYCCD`/`CloseQHYCCD`. Because the vendor SDK allows only one open camera at a time, the probe is skipped when any camera is already connected; in that case the wheel is attached optimistically and its `on_connect` fails with an explicit message if the camera turns out to have no wheel. Covered by `qsi_attaches_wheel_only_for_cameras_that_have_one` and `qsi_wheel_connect_fails_without_filter_wheel`. 
2. **Wheel moves publish `BUSY` and complete from a finalizer.** The original published `BUSY` and then blocked. Behavior as seen by a client is the same sequence of states, but the queue stays free and the wait is bounded (D2).
3. **Exposure readout polls from a bounded finalizer instead of a blocking loop** (D2), and the shared countdown is published (D5).
4. **`can_check_temperature` is preserved.** Temperature polling is still suppressed between exposure start and image handoff, because the original deliberately avoided SDK access during readout and `AGENTS.md` requires preserving that manner.
5. **`MAX_DEVICES` uses the generator default** instead of the original 128-entry `QSICamera::MAXCAMERAS` registry, as required by `AGENTS.md` ("never override `MAX_DEVICES` to preserve a legacy capacity"). Discovery still enumerates up to `QSICamera::MAXCAMERAS` serials.
6. **One hot-plug registration instead of two.** The generator registers a single libusb callback; both QSI product ids are accepted by filtering `descriptor.idProduct` inside `sdk.plug`. Observable discovery behavior is unchanged.
7. **The one-second sleep before re-enumeration is gone.** The generator's `discovery_retries` provides bounded, non-blocking retry for SDK enumeration that is not ready at USB arrival time, which is the same intent without sleeping on a callback.

8. **`CCD_TEMPERATURE` is `BUSY` only while the cooler is enabled.** The pre-migration driver published `BUSY` whenever the measured temperature differed from the target by more than 0.2 °C, including with the cooler off and with the default target of 0 °C. The generated change dispatch (`INDIGO_COPY_TARGETS_PROCESS_CHANGE`) rejects a request while the property is `BUSY`, so keeping the old condition would have made the target permanently unwritable. The condition now also requires `CCD_COOLER.ON`, matching `ccd_mi` and `ccd_qhy`. `qsi_sets_target_temperature` and `qsi_rejects_target_change_while_cooling` cover both sides.
9. **A failed disconnect no longer strands the device.** See defect D12.
10. **`CCD_FRAME` origin is reset to 0,0 at connect**, so a reconnect to a camera with a different sensor cannot inherit an out-of-range subframe. The pre-migration driver left the previous origin in place.
11. **`indigo_ccd_exposure_setup()` publishes the `CCD_IMAGE`/`CCD_IMAGE_FILE` BUSY states**, which the pre-migration driver published by hand just before setting `CCD_EXPOSURE` to `BUSY`. The set of published states is the same; only the relative order of these framework-owned updates changed.

## Reference trace comparison

`indigo_test/fixtures/ccd_qsi/original_reference_trace.txt` (pre-migration) against `indigo_test/fixtures/ccd_qsi/generated_reference_trace.txt` (migrated), both captured with `test_ccd_qsi_sdk --trace` and both byte-identical across two consecutive runs:

```diff
2a3,8
> get_Connected=false
> put_SelectCamera(00600123)
> put_Connected(true)
> get_HasFilterWheel=true
> get_FilterCount=5
> put_Connected(false)
4d9
< get_AvailableCameras=1
```

Those are the only two differences in the whole scenario:

- the six added lines are the discovery probe of difference 1;
- the removed `get_AvailableCameras` is the pre-migration driver's delayed re-enumeration of difference 7, which happened to land inside the connection window.

Every other SDK interaction — the complete connection capability sequence, the filter-wheel connection and move, the exposure setup, start, readiness poll and readout, all five control writes and the disconnect — is identical in content and in order.

## Coverage map

`indigo_test/integration/test_ccd_qsi_sdk.c`, 60 cases, run with `make -C indigo_test test-ccd-qsi-sdk`.

| `DRIVER_TESTING_RULES.md` area | Cases |
| --- | --- |
| Identity and lifecycle | `qsi_reports_driver_metadata`, `qsi_enumerates_common_properties_before_connect`, `qsi_connects_and_disconnects_ccd`, `qsi_tolerates_repeated_disconnect`, `qsi_repeats_init_and_shutdown`, `qsi_rejects_shutdown_while_connected`, `qsi_reports_wheel_slot_count` (wheel connected through the shared camera session), `qsi_serializes_sdk_access_across_logical_devices` |
| Discovery and ownership | `qsi_attaches_all_enumerated_cameras`, `qsi_ignores_duplicate_arrivals`, `qsi_handles_burst_arrivals`, `qsi_detaches_on_removal`, `qsi_removes_device_with_exposure_pending`, `qsi_rejects_foreign_vid_pid`, `qsi_accepts_both_product_ids`, `qsi_attaches_wheel_only_for_cameras_that_have_one`, `qsi_retries_discovery_when_enumeration_is_not_ready` |
| Initialization failures | `qsi_reports_connect_failure_and_recovers`, `qsi_reports_geometry_read_failure`, `qsi_recovers_from_disconnect_failure`, `qsi_rejects_second_camera_while_one_is_connected`, `qsi_wheel_connect_fails_without_filter_wheel` |
| Property contract | `qsi_publishes_ccd_class_properties`, `qsi_publishes_custom_properties`, `qsi_hides_unsupported_capabilities`, `qsi_builds_modes_for_linear_binning`, `qsi_builds_modes_for_power_of_two_binning`, `qsi_applies_exposure_limits_from_sdk`, `qsi_rebuilds_properties_on_reconnect`, `qsi_rebuilds_wheel_on_reconnect` |
| Controls | `qsi_writes_readout_speed`, `qsi_writes_anti_bloom`, `qsi_writes_pre_exposure_flush`, `qsi_writes_fan_mode`, `qsi_writes_gain`, `qsi_reports_control_write_failures` |
| Cooling and temperature | `qsi_polls_temperature_and_cooler_power`, `qsi_switches_cooler`, `qsi_sets_target_temperature`, `qsi_rejects_target_change_while_cooling`, `qsi_enables_cooler_when_target_is_set`, `qsi_reports_cooler_and_temperature_failures` |
| Image contract | `qsi_delivers_full_frame`, `qsi_delivers_subframe_with_binning`, `qsi_sends_shutter_flag_per_frame_type`, `qsi_uses_shortest_exposure_for_bias`, `qsi_reports_image_size_mismatch` |
| Acquisition | `qsi_publishes_exposure_countdown`, `qsi_retries_until_image_is_ready`, `qsi_rejects_overlapping_exposure`, `qsi_reports_exposure_start_failure`, `qsi_aborts_exposure_and_reacquires`, `qsi_times_out_stuck_readout` |
| Queue and callback races | `qsi_serializes_sdk_access_across_logical_devices`, `qsi_disconnects_while_readout_pending`, `qsi_makes_no_sdk_calls_after_close`, `qsi_removes_device_with_exposure_pending` |
| Additional interfaces (wheel) | `qsi_reports_wheel_slot_count`, `qsi_moves_wheel_to_first_last_and_intermediate_slots`, `qsi_reports_already_selected_slot`, `qsi_waits_for_wheel_to_settle`, `qsi_reports_wheel_move_failure`, `qsi_times_out_stuck_wheel`, `qsi_rebuilds_wheel_on_reconnect` |
| Streaming | **Not applicable.** The driver does not expose `CCD_STREAMING` and the vendor SDK has no live mode. |
| Guider | **Not applicable.** The driver never wires `PulseGuide()` to a logical guider device, so no guider interface is exposed and the guiding-pulse duration accuracy measurement of `DRIVER_TESTING_RULES.md` does not apply. |
| Configuration and strings | **Not covered, justified.** The four persistent custom properties are unconditionally re-read from the camera during every connection, exactly as before the migration, so a configuration round-trip cannot restore driver-specific SDK settings and there is nothing driver-owned to assert. The driver exposes no renameable strings or suffix controls. |

The fake SDK deliberately exercises both vendor error channels: it throws `std::runtime_error` while structured exceptions are enabled (the SDK default) and returns the QSI error code otherwise.

Two capability-hiding branches are **not** covered: `X_QSI_READOUT_SPEED` and `X_QSI_ANTI_BLOOM` are hidden only when the SDK reports a value outside `QSICamera::ReadoutSpeed` / `QSICamera::AntiBloom`. Both enumerations have no spare value inside their underlying range, so producing one is undefined behavior and UBSan rejects it. The equivalent branches for `X_QSI_FAN_MODE` and `X_QSI_PRE_EXPOSURE_FLUSH`, whose enumerations do have spare representable values, are covered by `qsi_hides_unsupported_capabilities`.

## Deferred work

- **Hardware acceptance.** No QSI camera available; the full hardware matrix of `DRIVER_TESTING_RULES.md` is untested and no hardware validation is claimed.
- **Apple Silicon hardware.** The `arm64` build is validated only against the fake SDK; no QSI camera was connected to an Apple Silicon host.
- **Windows.** No QSI Windows SDK is bundled; unchanged from the baseline.
- **Linux architectures.** Only macOS `x86_64` and `arm64`-hosted cross-compilation were exercised here. The Linux `arm`, `arm64`, `x86` and `x64` SDK archives were not linked in this environment.

## Final test summary

Commands actually executed on macOS 26.6.2 / Apple clang 21.0.0, Apple Silicon host. The driver and its test are now built as universal `x86_64`/`arm64` binaries:

- `make -C indigo_libs libftd2xx` from a removed `build/lib/libftd2xx.a` — installs D2XX 1.4.35 as `x86_64 arm64 arm64e` with zero embedded libusb members left in any slice.
- `make -C indigo_drivers/ccd_qsi -f ../../Makefile.drv` — PASS, no compiler diagnostics and, unlike the baseline, no architecture-mismatch linker warnings.
- `../../build/bin/indigo_generator indigo_ccd_qsi.driver` re-run over the checked-in output — no diff, generation is reproducible.
- `make -C indigo_test test-ccd-qsi-sdk` — 60 of 60 cases pass (runs the `arm64` slice on this host).
- `arch -x86_64 build/integration/test_ccd_qsi_sdk` — 60 of 60 cases pass, so the Intel slice is unaffected by the vendor-archive change.
- The same suite built `-arch arm64 -fsanitize=address,undefined` with `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1` — 60 of 60 cases pass, no sanitizer findings.
- Pre-migration characterization run of the same suite against the unmodified driver — 46 of 48 cases passed, the two failures being the recorded reproducers for D11 and D12.
- `test_ccd_qsi_sdk --trace` on both drivers, twice each — traces stable, differences as recorded in [Reference trace comparison](#reference-trace-comparison).

Totals:

- Simulated (hardware-free) tests run: **60**; passed: **60**. Counted once; the suite was additionally executed on the `x86_64` slice and under sanitizers with the same result.
- Hardware tests run: **0**; passed: **0**.

## 2026-09-22 macOS Intel-only correction

### Scope and audit

The user clarified that the QSI driver is supported on Intel macOS only. The current generated driver version 17 has no `supported_architecture` attribute and therefore compiles the full SDK-backed implementation for both macOS slices. Linux support remains unchanged. This correction is limited to generated architecture metadata and does not change properties, SDK calls, lifecycle behavior or test coverage.

The generator migration guide documents the exact platform-specific expression required here: `!defined(INDIGO_MACOS) || defined(__x86_64__)`. On macOS arm64 the generated entry point must retain INFO metadata and return `INDIGO_UNSUPPORTED_ARCH` for INIT and SHUTDOWN; non-macOS targets must retain the implementation.

No QSI hardware is available and no hardware testing will be performed for this correction. Hardware tests run: **0**; passed: **0**.

### Baseline

- `make -f ../../Makefile.drv` from `indigo_drivers/ccd_qsi` — PASS for the unmodified version 17 universal driver. The build emits the existing duplicate-libusb, deployment-target and libc++ support warnings; there are no errors.

### Atomic plan

| # | Step | Verification | State | Result |
| - | ---- | ------------ | ----- | ------ |
| 1 | Read the repository, driver and generator instructions and audit the current architecture metadata. | Current source and generated entry point inspected. | DONE | Version 17 has no restriction and no unsupported-architecture fallback. |
| 2 | Build the unmodified driver as the baseline. | `make -f ../../Makefile.drv` succeeds. | DONE | PASS with the existing warnings recorded above. |
| 3 | Add the macOS Intel-only expression to the `.driver` source and raise the driver version. | Source contains the documented generator attribute and a higher version. | DONE | Added `!defined(INDIGO_MACOS) || defined(__x86_64__)`; version raised from 17 to 18. |
| 4 | Regenerate the checked-in `.cpp`, `.h` and `_main.c` outputs. | A second generator run is byte-identical. | DONE | Generator emitted the version/architecture guard and current connection/timer/unplug semantics; SHA-1 hashes were identical after the second run. The header and standalone wrapper remained unchanged. |
| 5 | Build and inspect both macOS slices and run an arm64 unsupported-architecture contract check. | Intel slice contains the implementation, arm64 contains the unsupported stub, and the stub returns the documented results. | DONE | Universal build passed. `nm` shows `QSICamera` references only in x86_64. The arm64 contract test passed: INFO reports version 18; INIT and SHUTDOWN return `INDIGO_UNSUPPORTED_ARCH`. |
| 6 | Run the existing 60-case fake-SDK suite on the supported x86_64 slice. | 60/60 tests pass. | BLOCKED | The universal test binary built successfully, but this Apple Silicon host has no Rosetta and `arch -x86_64 ...` fails with `Bad CPU type in executable`. No test case started. |

### Found defect

| ID | Evidence class | Impact | Root cause | Fix | Regression verification |
| -- | -------------- | ------ | ---------- | --- | ----------------------- |
| D13 | User report and source audit | The macOS arm64 slice advertises and initializes a driver that is not supported on that platform. | The prior Apple Silicon experiment removed `supported_architecture`, but QSI support remains Intel-only. | Restored the generator-owned macOS Intel-only guard and bumped the driver version to 18. | Slice inspection and the arm64 unsupported-architecture contract test pass. The x86_64 suite compiled but could not run because Rosetta is unavailable. |

### Final test summary

- `make -f ../../Makefile.drv` — PASS. Both macOS slices compile and link; only the existing duplicate-libusb, deployment-target and libc++ support warnings remain.
- Two consecutive `indigo_generator indigo_ccd_qsi.driver` runs — PASS, byte-identical generated output.
- `nm -arch arm64 build/drivers/indigo_ccd_qsi.a` — only `indigo_ccd_qsi` is present; no `QSICamera` references.
- `nm -arch x86_64 build/drivers/indigo_ccd_qsi.a` — full implementation and `QSICamera` references are present.
- Arm64 unsupported-architecture contract test — PASS: INFO returns OK with version 18; INIT and SHUTDOWN return `INDIGO_UNSUPPORTED_ARCH`.
- `make build/integration/test_ccd_qsi_sdk` — PASS for the universal test binary.
- `arch -x86_64 build/integration/test_ccd_qsi_sdk` — unavailable: Rosetta is not installed (`Bad CPU type in executable`), so no suite case ran.

Totals for this correction:

- Simulated (hardware-free) tests run: **1**; passed: **1** (architecture contract). The existing 60-case Intel suite was built but not executed.
- Hardware tests run: **0**; passed: **0**.

## Regeneration with the current generator (2026-09-24)

A full build regenerated `indigo_ccd_qsi.cpp` differently from the checked-in output: the checked-in file predated the current generator semantics. Regenerating from the unchanged `.driver` source changes three generator-owned parts: the camera's periodic `ccd_timer_callback` is started after `CONNECTION` is published instead of before, the camera and wheel `CONNECTION` changes go through `INDIGO_PROCESS_QUEUED_CONNECT`, and USB removal only runs the SDK serial match when the removed device does not already match directly. The version was raised from 18 to 19 for this behaviour change and the outputs were regenerated.

Validation: `make -C indigo_drivers/ccd_qsi -f ../../Makefile.drv all` and the 60-case fake SDK suite `indigo_test/build/integration/test_ccd_qsi_sdk` on Linux x86_64: 60 run, 60 passed. macOS, Windows and physical QSI cameras were not run.

- Simulated (fake SDK) tests run: **60**; passed: **60**.
- Hardware tests run: **0**; passed: **0**.
