# Dome Simulator test-coverage audit

Status: coverage extension complete on 2026-09-13. Driver version: `0x03000007`.

## Current-state audit

`Dome Simulator` is a generated, virtual, single-device dome driver. `indigo_dome_simulator.driver` is the source of truth and `.c`, `.h` and `_main.c` are generated outputs. It has no serial/network transport, manufacturer protocol, SDK, firmware or physical mechanism. Queue handlers simulate absolute and relative azimuth motion, clockwise/counterclockwise wrap, park/unpark, a delayed shutter, abort and disconnect cancellation. Position, target and operation state live in private data and are serialized by the device queue.

The visible connected contract is `DOME_STATE`, `DOME_SPEED`, `DOME_DIRECTION`, `DOME_ON_COORDINATES_SET`, `DOME_STEPS`, `DOME_HORIZONTAL_COORDINATES`, `DOME_SLAVING_PARAMETERS`, `DOME_ABORT_MOTION`, `DOME_SHUTTER`, `DOME_PARK`, `DOME_DIMENSION` and `GEOGRAPHIC_COORDINATES`. Flap, park-position, home, UTC and host-time properties remain hidden and unsupported. No property is being added or removed, so `indigo_docs/PROPERTIES.md` does not need an update.

The existing six-case suite covers metadata/property visibility, representative property items/ranges, shutter open/close, GOTO completion, park/unpark, relative wrap in both directions, active abort, queued-start abort, pending disconnect, stale-position exclusion and reconnect recovery. Gaps found against the concrete exposed contract are: complete item/range assertions for state, on-coordinate-set, slaving, dimension and geography; state-light transitions; absolute GOTO rejection while parked; overlap policy; idle abort; and a successful fresh move after abort. There is no parser, I/O or SDK boundary, so malformed replies, transport errors and hardware error injection are not applicable.

The generated driver is portable C and is integrated in Unix/macOS build, Xcode and Visual Studio projects. The current environment can build/run arm64 and x86_64 macOS binaries; Linux and Windows execution are unavailable.

## Baseline evidence

Before test or production edits, the current worktree driver and test were rebuilt with:

```text
make -B -C indigo_drivers/dome_simulator -f ../../Makefile.drv all
make -B -C indigo_test build/integration/test_dome_simulator
indigo_test/build/integration/test_dome_simulator
```

The universal x86_64/arm64 archive, dynamic library and executable built successfully. Native arm64 integration passed **6/6** cases.

## Hardware-test decision

No hardware testing will be performed. This driver is itself an in-process software simulator and supports no hardware model. Hardware tests planned/run/passed: **0/0/0**.

## Atomic plan and results

1. **Read instructions, generated source, class rules and existing tests; establish the baseline — completed.** The fresh universal build and native integration run passed 6/6.
2. **Map supported behavior and identify concrete coverage gaps — completed.** Seven gaps are listed in the audit; transport/protocol/SDK/hardware cases are explicitly non-applicable.
3. **Extend public-bus coverage without changing production behavior — completed.** The suite now has seven cases. Existing compliance coverage asserts every exposed state/on-set/slaving/dimension/geography item and stable numeric range, plus shutter/slew/park light transitions. The parked test now rejects both absolute and relative moves. New `simulator_overlap_idle_abort_and_recovery` proves BUSY overlap preserves the first target, idle abort clears its switch, active abort reaches a terminal coordinate/state condition and a fresh move succeeds. The first compile used a nonexistent cleanup macro and was corrected to the harness's `SERIAL_CHECK_TRUE`; the first run then exposed that abort intentionally publishes an ALERT slew light rather than IDLE, so the acceptance assertion was corrected to the actual terminal requirement, not BUSY. The corrected native run passed **7/7**.
4. **Run generated-output, strict, sanitizer and available-architecture validation — completed.** Regeneration left `.c`, `.h` and `_main.c` unchanged. Strict `-Wall -Wextra -Werror` syntax checks passed for arm64 and x86_64. Corrected native arm64, native arm64 ASan, normal x86_64 Rosetta and x86_64 Rosetta ASan each passed 7/7. Test and generated driver source were sanitizer-instrumented, while shared `libindigo` was not.
5. **Update integration/status records and perform final hygiene — completed.** This ledger is registered in the Xcode driver group. `MIGRATION_STATUS.md` now records simulator retesting while preserving Comment and every other status cell. Xcode plist validation, `git diff --check`, no-production-diff verification and README exclusion passed. Test and driver build artifacts were removed with the documented clean targets.

## Scenario-to-test mapping

| Supported area | Current coverage and planned extension |
| --- | --- |
| Metadata and exact property visibility | `driver_info_reports_simulator_metadata`, `simulator_exposes_expected_properties` |
| Interface, items and numeric ranges | `simulator_passes_dome_compliance_checks`; extend to every exposed property/item and stable range |
| Absolute/relative motion and wrap | Compliance plus `simulator_relative_wrap_and_park_guard`; add overlap/target preservation |
| Park and parked guards | Compliance and relative guard; add absolute GOTO guard and state-light checks |
| Shutter | Compliance and pending disconnect; add open-state light checks |
| Abort and recovery | Compliance plus `simulator_queued_abort`; add idle abort and fresh successful move |
| Disconnect/reconnect and stale work | `simulator_pending_disconnect` |

## Validation evidence

- Universal x86_64/arm64 driver build: passed.
- Native arm64 baseline: **6/6 passed**.
- First expanded native run: **6/7 passed** and exposed the documented terminal state expectation; corrected without a production change.
- Corrected native arm64 integration: **7/7 passed**.
- Native arm64 AddressSanitizer: **7/7 passed**.
- Normal x86_64 Rosetta: **7/7 passed**.
- AddressSanitizer x86_64 Rosetta: **7/7 passed**.
- Generated output reproducibility, strict arm64/x86_64 checks, Xcode plist and diff hygiene: passed.
- Linux and Windows execution: unavailable and not claimed.

## Final test summary

Simulated tests run: **41**. Simulated tests passed: **40**. The one failed execution was the recorded pre-correction expanded test; the final corrected matrix passed **28/28**. Hardware tests run: **0**. Hardware tests passed: **0**.
