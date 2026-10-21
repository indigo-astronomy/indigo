# AUX Simulator implementation

## Baseline and audit

New virtual INDIGO 3.0 driver; there is no original driver, migration trace or baseline suite to preserve. The existing generator and framework archives are available on macOS arm64. The working tree was clean before this task. References: DRIVER_DEVELOPMENT_BASICS.md, DRIVER_GENERATOR_MIGRATION.md, PROPERTIES.md, MAKEFILES.md, AUX driver acceptance rules, aux_upb, aux_sqm and aux_wcv4ec.

Four logical AUX devices share the generated lifecycle and serialized master queue, with distinct property handles. No physical transport, SDK or hardware resource is involved. Public standard properties model a powerbox (four power outlets, two heater channels, four USB ports, manual/automatic dew control and derived telemetry), weather station (readings and condition lights), SQM (sky brightness, temperature and derived Bortle class), and flat panel (light, intensity and asynchronous cover). Custom X_SIMULATION_PARAMETERS supply deterministic weather and SQM readings for client debugging. The flat panel retains its last completed position when disconnected during motion; a delayed finalizer must never survive disconnect.

Portable generated C targets Linux, macOS and Windows. Makefile.drvs discovers the directory automatically; server and Xcode registration, generated files, property documentation and test registration are part of this change. No generator change is planned. Existing drivers and A1 are outside scope.

## Hardware decision

No hardware tests: all four devices are virtual. Linux and Windows runtime validation are unavailable on this host. No hardware validation will be claimed.

## Atomic plan

1. Complete: read rules, inspect reference drivers/build/test infrastructure, establish new-driver baseline and record design.
2. Complete: implemented .driver and generated C/header/main. Universal macOS x86_64/arm64 archive, dynamic library and standalone executable built with `make -C indigo_drivers/aux_simulator -f ../../Makefile.drv all`. Strict `clang -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unknown-pragmas -fsyntax-only` passed. Device code blocks were moved into the supported shared `code` block because the existing generator emits device code only when a shared code/queue condition is met; no generator edits were made.
3. Complete: 13 integration cases pass on macOS arm64, including all four devices, properties, telemetry, condition changes, lifecycle/reconnect, sibling isolation, cover BUSY/refusal/disconnect recovery and all four Auxiliary Agent slots. The same 13 cases pass with AddressSanitizer and UndefinedBehaviorSanitizer (`ASAN_OPTIONS=detect_leaks=0 indigo_test/build/integration/test_aux_simulator_asan`). No sanitizer diagnostics.
4. Complete: registered sources/tests/docs in Xcode, the static server, the stable-driver list and the Windows solution/project; documented all public properties, formulas and lifecycle behavior. README creation was explicitly approved by the user. Concurrent Xcode additions were preserved, and duplicate AUX simulator registration was consolidated. Both Windows XML files parse successfully and the solution includes all four Debug/Release x64/ARM64 configurations; native Windows compilation remains unverified.
5. Complete: generator C/header/main reproduce byte for byte. `plutil -lint indigo.xcodeproj/project.pbxproj` and whitespace checks pass (respecting the existing solution CRLF). Final recorded run: `python3 tools/run_driver_test.py aux_simulator`, 13/13 OK, 3.0.0.1, mac arm64; README and TEST_SUMMARY.md updated by the script. Final project audit confirms every persistent file is registered once with no duplicate native compilation. `make -C indigo_test test-clean` removed test binaries. The driver change is prepared as an isolated commit; unrelated concurrent Xcode edits are excluded.

## Found defects

No production-driver defects were found in the final validation. This driver makes the separately identified A1 cover-toggle issue reproducible and does not modify A1.

Initial diagnostic test failures came from test-harness assumptions: the historical defined-property list retains names after deletion (use the live property cache to verify deletion); reselecting an already connected sibling must use enumeration rather than wait for an ignored duplicate connect to emit an update; the current framework BUSY guard silently ignores overlapping changes (test unchanged target and eventual completion rather than expect a refusal message). The corrected cases pass without changing framework behavior.

## Coverage map and limits

- `metadata`: driver name, label, lifecycle metadata and multi-device flag.
- Four `*_inventory` cases: disconnected/connected property inventories and repeated disconnect.
- `lifecycle_and_interfaces`: independent connection, interface bits, refused SHUTDOWN while connected, property deletion and reconnect for every device.
- `powerbox_outputs_and_telemetry`: power and USB changes, matching state/current, aggregate power/current, automatic dew setting, manual override and retained state after reconnect.
- `weather_readings_and_conditions`: deterministic input/readback, derived dew point (including zero and saturated humidity), wind/rain/cloud warning and wind recovery.
- `sqm_readings_and_bortle`: editable brightness and temperature, derived Bortle class, retained values after reconnect.
- `flat_cover_and_light`: one-of-many cover, both explicit CLOSE and OPEN=false, repeated movement, light and intensity.
- `flat_busy_refusal_and_disconnect`: overlapping request cannot replace motion, disconnect cancels finalizer, no updates after delete, reconnect at last completed position and fresh subsequent motion.
- `siblings_remain_independent`: all four connected together, disconnecting the master PowerBox does not cancel a flat-panel motion, SHUTDOWN still refused while a sibling is connected.
- `auxiliary_agent_mirrors_all_devices`: all four slot lists, standard mirrored properties and open/close through the agent, including OPEN=false.

No transport, protocol, SDK, unplug, timeout injection or guider accuracy scenarios apply to these virtual devices. No abort property is exposed. Delayed cover completion runs on the existing serialized queue and is cancelled by the generated disconnect lifecycle. Values are deterministic and input-driven; no stochastic/time-varying weather model is claimed. Windows/Linux runtime tests, Xcode application builds and live A1 UI validation have not been run.

## Final test summary

Final validation: simulated tests 26 run, 26 passed (13 recorded regular cases + 13 ASan/UBSan cases). Hardware tests: 0 run, 0 passed. Earlier development/diagnostic reruns are excluded from these final-validation totals.
