# rotator_lunatico refactoring record

`indigo_rotator_lunatico` is the rotator entry point of the Lunatico Astronomia
Limpet / Seletek / Armadillo / Platypus controller driver. It has no implementation of its
own: it sets `DEFAULT_DEVICE = TYPE_ROTATOR` and includes
`../focuser_lunatico/shared/lunatico_shared.c`, which `indigo_focuser_lunatico` includes as
well. Both drivers are migrated in the same change.

**The shared audit, protocol reference, migration design, atomic plan and found-defects
record live in `../focuser_lunatico/REFACTOR.md`.** This file records only what is specific to
the rotator driver. Read both together.

## Scope and baseline

Baseline date and source: 2026-09-19, commit `1d370a920`
(`aux_dragonfly/dome_dragonfly: migrated to code generator`), branch `refactoring`.

Host: macOS 26.6.2 (`Darwin 25.6.0`), Apple Silicon arm64; repository universal build
(x86_64 + arm64).

Baseline build:

```sh
cd indigo_drivers/rotator_lunatico
make -f ../../Makefile.drv all
```

Result: passed. Archive, dynamic library and standalone executable built for x86_64 and
arm64. The bare `make -f ../../Makefile.drv` does not build the driver; see defect LU-07 in
the shared record.

Baseline automated tests:

```sh
cd indigo_test
make build/integration/test_rotator_lunatico_simulator
./build/integration/test_rotator_lunatico_simulator
```

Result: 11 scenarios run, 11 passed, exit `0`
(`main_rotator`, `exp_rotator`, `third_rotator`, `exp_focuser`, `third_powerbox`, `abort`,
`goto_failure`, `read_failure`, `wrong_model`, `silent`, `oversized`).

`MIGRATION_STATUS.md` records `7 / 0` and `Retested ❌ No` for this driver. Both are stale:
the suite has eleven scenarios and passes in full. The row is corrected by this change.

The suite has a build rule in `indigo_test/Makefile` but is **not** listed in
`INTEGRATION_TESTS`, so `make -C indigo_test test` never runs it. There is also an opt-in
UDP variant, `test_rotator_lunatico_udp_simulator`, reachable through
`make -C indigo_test test-rotator-lunatico-udp`, which builds the same test source against a
`SIM_UDP=1` build of the simulator. Both are addressed in step 7 of the shared plan.

## Hardware-test decision

No hardware testing will be performed. The user has stated that no Lunatico controller is
available. Nothing in this record implies physical validation; every result is
simulator-backed software behaviour.

## Rotator-specific audit

### What this entry point changes

`DEFAULT_DEVICE = TYPE_ROTATOR` makes the Main port come up as a rotator instead of a
focuser. `CONFLICTING_DRIVER` is `indigo_focuser_lunatico`, so loading one driver refuses the
other. `DRIVER_INFO` is `"Lunatico Astronomia Rotator"`. `DRIVER_VERSION` is `0x0200000B`
against the focuser driver's `0x0200000C`. Everything else is the shared implementation.

### Rotator device behaviour

- Angles are converted to steps against `ROTATOR_STEPS_PER_REVOLUTION` (100…100000, default
  3600) with `ROTATOR_LIMITS_MIN_POSITION` as the zero offset, by `degrees_to_steps()` and
  `steps_to_degrees()`.
- `ROTATOR_LIMITS` defaults to -180…180 over a -180…360 range. When the two limits map to the
  same step value the driver deletes the controller's software limits, otherwise it writes
  them with `!step setswlimits#`. A requested position outside the limits is rejected with
  `INDIGO_ALERT_STATE` and the message `Requested position is not in the limits.`, and the
  previous angle is restored.
- `ROTATOR_ON_POSITION_SET` selects between `!step goto#` and `!step setpos#`. A goto to the
  angle the driver already believes it is at is answered `INDIGO_OK_STATE` with no command.
- `ROTATOR_DIRECTION` and the motor wiring property both map onto `!step wiremode#`, combining the
  Lunatico/Moonlite wiring with the normal/reversed direction into one of four values.
- Changing `ROTATOR_LIMITS` or `ROTATOR_STEPS_PER_REVOLUTION` re-syncs the controller's
  position counter to the current angle through `lunatico_sync_to_current()`, because both
  change the degree-to-step mapping.
- `ROTATOR_ABORT_MOTION` issues `!step stop#`, reads the position back and publishes it.

### Rotator-specific defects

`LU-06` in the shared record is rotator-only: the goto poll is armed through
`PORT_DATA.focuser_timer` while abort and disconnect cancel `PORT_DATA.rotator_timer`, so
neither can stop the poll a goto started and it outlives the connection. It is visible in the
driver's own log, where `lunatico_is_moving(0) failed` and `NO response` appear after a clean
disconnect, but it is not reproducible through public bus APIs; the shared record states why.

`LU-08`, the reported angle moving when only the limits change, was found by comparing the
pre-migration and post-migration reference traces of this driver and is reproduced by
`limits_change_keeps_angle`.

`DRV-210`, recorded in the existing `test_rotator_lunatico_simulator.c` comments, was fixed in
commit `b9f894420`: connect now seeds `PORT_DATA.r_current_position` from the angle read back
from the controller, so a first request for an angle the driver had not yet read is no longer
silently swallowed. The `motion_case` scenario covers it and passes.

### Rotator coverage gaps at baseline

Absent from `test_rotator_lunatico_simulator.c` and required by the Rotator class standard in
`indigo_test/DRIVER_TESTING_RULES.md`: `ROTATOR_STEPS_PER_REVOLUTION` changes and the
re-sync they trigger, `ROTATOR_LIMITS` changes including the delete-limits boundary and the
out-of-limits rejection, `ROTATOR_DIRECTION` normal/reversed and the `wiremode` value it
sends, `ROTATOR_BACKLASH`, abort while idle, `stop-error`, disconnect during motion, both
connection orders for a shared port, sibling survival on last close, and every `LA_*`
property.

## Step evidence

### Step 4 — rotator_lunatico characterization suite

`indigo_test/integration/test_rotator_lunatico_simulator.c` grows from 11 to **27 scenarios**.
Shared helpers live in the new `indigo_test/integration/lunatico_test_common.h`, which the
focuser suite uses too. The suite was also added to `INTEGRATION_TESTS`, where it had been
missing, so `make -C indigo_test test` now runs it.

This suite owns the rotator class coverage plus the proof that the focuser and powerbox
classes are reachable from this entry point; the focuser suite owns the identity, powerbox,
shared-connection and settings coverage of the common code, which is not duplicated here.

| Requirement (Rotator class standard) | Scenario |
| --- | --- |
| Interface bit, class property completeness, unhidden rotator properties and their documented defaults, connect sequence including the counter re-sync | `identity_and_inventory` |
| Degree-to-step mapping and absolute GOTO on each of the three ports, with BUSY, measured progress and the exact command | `main_goto`, `exp_goto`, `third_goto` |
| Wrap past the revolution boundary | `goto_wraps` |
| Zero/no-op GOTO issues no command | `no_op_goto` |
| SYNC writes the counter without a move | `sync_without_moving` |
| Abort in motion, measured stopped angle, fresh move afterwards | `abort_motion` |
| Abort while idle | `abort_while_idle` |
| Steps per revolution re-maps and re-syncs the counter | `steps_per_revolution` |
| Limits written as controller software limits, driver-side rejection outside them, accepted target inside them | `limits` |
| Full-range limits delete the controller's limits | `full_range_limits` |
| A limits change does not move the reported angle | `limits_change_keeps_angle` |
| Direction and motor wiring combined into one `wiremode` value | `direction` |
| Backlash passed to absolute GOTO | `backlash` |
| Start, SYNC, stop and readback failures, and a partial settings write failure | `goto_failure`, `sync_failure`, `stop_failure`, `read_failure`, `limits_failure` |
| Identity refusal | `wrong_model`, `silent`, `oversized` |
| Disconnect/reconnect | `reconnect` |
| Disconnect during motion, no traffic after the close, correct angle on the next connection | `disconnect_during_motion` |
| A focuser and a powerbox on secondary ports of this entry point | `exp_focuser`, `third_powerbox` |

Result against the **unchanged** driver: 26 of 27 passed;
`limits_change_keeps_angle` failed, which is the dedicated reproducer of defect LU-08 recorded
in the shared record. Against the migrated driver all 27 pass.

Deliberately not covered here, with justification: `ROTATOR_RELATIVE_MOVE`,
`ROTATOR_RAW_POSITION` and `ROTATOR_POSITION_OFFSET` stay hidden because the driver does not
implement them; hand-controller movement is not polled by this driver; and everything the
focuser suite already covers through the same shared code is not duplicated.

### Step 6 — rotator_lunatico migrated to the generator

New `indigo_rotator_lunatico.driver` (972 lines) over the same shared implementation, with
`rotator_main` on the Main port in place of the focuser. `Makefile.inc` is deleted. Both
definitions declare `driver lunatico`, so both generated drivers expose `lunatico_private_data`,
`lunatico_open()` and `lunatico_close()` and the shared file stays a single implementation.

The rotator devices declare the four per-port settings as `X_ROTATOR_STEP_MODE`,
`X_ROTATOR_POWER_CONTROL`, `X_ROTATOR_MOTOR_WIRING` and `X_ROTATOR_MOTOR_TYPE`, with
port-suffixed C handles; there is no temperature sensor selection on a rotator.

```sh
cd indigo_drivers/rotator_lunatico
../../build/bin/indigo_generator indigo_rotator_lunatico.driver
make -B -f ../../Makefile.drv
```

Result: generated and built for x86_64 and arm64, 0 lines containing `warning` or `error`.
Regeneration is reproducible byte for byte.

Reference traces: 11 of the 12 captured rotator traces are byte identical to the
pre-migration ones. The single difference, `limits.txt`, is the LU-08 fix.

### Steps 7 and 8 — integration and verification

Rotator-specific results, with the shared details in `../focuser_lunatico/REFACTOR.md`:

- `indigo_rotator_lunatico.vcxproj`, `.filters` and `.user` added and registered in
  `indigo_windows.sln`. No Windows build was performed.
- `rotator_lunatico_simulator/rotator_lunatico_simulator.c` is now a thin wrapper around the
  shared `lunatico_simulator_common.h`; the `.driver`, the `REFACTOR.md`, the simulator group
  and the Windows project were added to the Xcode project.
- `MIGRATION_STATUS.md` row advanced to 3️⃣ with `27 / 0`, replacing the stale `7 / 0` and
  `Retested ❌ No`.
- AddressSanitizer: 27 of 27 scenarios pass with no sanitizer report.
- UDP transport: `make -C indigo_test test-rotator-lunatico-udp` runs the whole suite over a
  UDP simulator, 27 of 27 passed.
- `README.md` is **not** updated; it documents the port configuration this change removes and
  the root instructions require the user's explicit approval to change a `README.md`.

## Final test summary

- Simulated tests: **27 executed, 27 passed** against the migrated driver. The same 27 were
  run against the original driver, where 26 passed and `limits_change_keeps_angle` failed as
  the dedicated LU-08 reproducer. Additional runs of the same 27 scenarios: 27 passed under
  AddressSanitizer with no sanitizer report, and 27 passed over the UDP transport.
- Hardware tests: **0 executed, 0 passed.** No Lunatico controller was available and no
  hardware validation is claimed.
