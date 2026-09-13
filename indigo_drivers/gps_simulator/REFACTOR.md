# GPS Simulator test-coverage audit

Status: coverage audit complete on 2026-09-13. Driver version: `0x03000009`.

## Current-state audit

`GPS Simulator` is a generated, virtual, single-device GPS driver. `indigo_gps_simulator.driver` is the source of truth and the checked-in `.c`, `.h` and `_main.c` files are generated outputs. It has no serial/network transport, manufacturer protocol, SDK, firmware or physical resource. Its queue callback publishes a deterministic lifecycle shape with randomized values inside fixed ranges: initial no-fix data, a 2D fix after ten one-second ticks, a 3D fix after twenty ticks, current UTC, coordinates/accuracy, satellite counts and DOP values. `GPS_ADVANCED` controls definition of `GPS_ADVANCED_STATUS`. Disconnect cancels the recurring handler and reconnect starts a fresh no-fix session.

The visible connected property contract is `GEOGRAPHIC_COORDINATES` with latitude, longitude, elevation and accuracy, `UTC_TIME.TIME`, `GPS_STATUS` with no-fix/2D/3D lights, and `GPS_ADVANCED`; `GPS_ADVANCED_STATUS` is defined only while advanced reporting is enabled. Common connection/configuration/simulation/additional-instance properties remain framework-owned. No property is being added or removed, so `indigo_docs/PROPERTIES.md` does not need an update.

The existing four-case integration suite exercises metadata, exact property visibility, the GPS class contract and ranges, advanced-property visibility, no-fix/2D/3D transitions and values, shutdown rejection while connected, disconnect cancellation, absence of post-disconnect updates and fresh reconnect state. Protocol framing, malformed input, checksums, read/write failures, receiver silence and transport loss are not applicable because this virtual simulator has no parser or I/O boundary. Concurrent/multi-device ownership is not applicable to its single logical device. Generic numeric validation and UTC formatting are framework/library concerns.

The generated driver is portable C and is integrated in the Unix/macOS build, Xcode and Visual Studio projects. The current environment can build/run arm64 and x86_64 macOS binaries; Linux and Windows execution are unavailable.

## Baseline evidence

Before test or production edits, the current worktree driver and test were rebuilt with:

```text
make -B -C indigo_drivers/gps_simulator -f ../../Makefile.drv all
make -B -C indigo_test build/integration/test_gps_simulator
indigo_test/build/integration/test_gps_simulator
```

The universal x86_64/arm64 archive, dynamic library and executable built successfully. Native arm64 integration passed **4/4** cases.

## Hardware-test decision

No hardware testing will be performed. This driver is itself an in-process software simulator and supports no hardware model. Hardware tests planned/run/passed: **0/0/0**.

## Atomic plan and results

1. **Read instructions, generated source, class rules and existing tests; establish the baseline — completed.** The fresh universal build and native integration run passed 4/4.
2. **Map every supported behavior and failure/lifecycle path to named tests — completed.** The existing suite covers every applicable GPS simulator branch. All omitted GPS protocol/reader cases require an external input boundary that this driver intentionally does not have; no new functional test is justified.
3. **Run generated-output, strict, sanitizer and available-architecture validation — completed.** Regeneration left `.c`, `.h` and `_main.c` unchanged. Strict `-Wall -Wextra -Werror` syntax checks passed for arm64 and x86_64. Native arm64 ASan, normal x86_64 Rosetta and x86_64 Rosetta ASan each passed 4/4; the baseline native arm64 run also passed 4/4. Test and generated driver source were sanitizer-instrumented, while shared `libindigo` was not.
4. **Update integration/status records and perform final hygiene — completed.** This ledger is registered in the Xcode driver group. `MIGRATION_STATUS.md` now records simulator retesting while preserving Comment and every other status cell. Xcode plist validation, `git diff --check`, no-production-diff verification and README exclusion passed. Test and driver build artifacts were removed with the documented clean targets.

## Scenario-to-test mapping

| Supported area | Named coverage |
| --- | --- |
| Metadata and property visibility | `driver_info_reports_simulator_metadata`, `simulator_exposes_expected_properties` |
| GPS interface, items and numeric ranges | `simulator_passes_gps_compliance_checks` |
| Advanced visibility and data | `simulator_passes_gps_compliance_checks`, `simulator_fix_lifecycle_and_reconnect` |
| No-fix, 2D, 3D, coordinates and UTC | `simulator_fix_lifecycle_and_reconnect` |
| Connected shutdown rejection | `simulator_fix_lifecycle_and_reconnect` |
| Disconnect cancellation, stale-update exclusion and reconnect reset | `simulator_fix_lifecycle_and_reconnect` |

## Validation evidence

- Universal x86_64/arm64 driver build: passed.
- Native arm64 integration baseline: **4/4 passed**.
- Native arm64 AddressSanitizer: **4/4 passed**.
- Normal x86_64 Rosetta: **4/4 passed**.
- AddressSanitizer x86_64 Rosetta: **4/4 passed**.
- Generated output reproducibility, strict arm64/x86_64 checks, Xcode plist and diff hygiene: passed.
- Linux and Windows execution: unavailable and not claimed.

## Final test summary

Simulated tests run: **16**. Simulated tests passed: **16**. Hardware tests run: **0**. Hardware tests passed: **0**.
