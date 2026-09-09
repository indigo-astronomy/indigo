# AUX protocol simulator tests

These tests exercise unchanged production drivers through the public INDIGO bus and real PTY/UDP I/O. No hardware, SDK substitutes or driver I/O overrides are used. Faults are intentionally injected by separate simulator processes. Each scenario owns a fresh simulator; its parent reaps it even if the driver crashes or exceeds the 35-second watchdog. A passing data assertion does not hide failed disconnect or shutdown.

## Sources and precedence

Use manufacturer protocol documentation as the primary oracle. Existing simulators and production drivers supply supplementary information only; undocumented details must be identified rather than presented as independent protocol validation.

| Driver | Primary source | Supplementary information |
| --- | --- | --- |
| `aux_cloudwatcher` | [Lunático protocol v1.4](https://lunaticoastro.com/rs232-communication-protocol-cloudwatcher/), [v1.3](https://lunaticoastro.com/aagcw/TechInfo/Rs232_Comms_v130.pdf), [v1.1 binary constants](https://lunaticoastro.com/aagcw/TechInfo/Rs232_Comms_v110.pdf), [v1.0 framing](https://lunaticoastro.com/aagcw/TechInfo/Rs232_Comms_v100.pdf) | Driver property names and supported commands. Fixture models firmware 5.89 with SQ, humidity and pressure sensors. |
| `aux_dragonfly` | [Seletek developer guide](https://lunaticoastro.com/seletek-developers-guide/) for SLP framing, UDP and channel ranges; [Dragonfly JavaScript API](https://lunaticoastro.com/df-javascript/) for pulse units and behavior | Existing `indigo_drivers/aux_dragonfly/relio_simulator/relio_simulator.pl` supplies detailed reply forms. The public guide defers the full command catalogue to a spreadsheet available on request; these reply details remain supplementary. |
| `aux_mgbox` | Repository `indigo_drivers/aux_mgbox/doc/MGPBox Manual English 1.1.pdf`, pages 15–20 | Driver sample for the device-type reply text, which the manual does not spell out. GPS fixtures use conventional NMEA RMC/GGA layout and computed checksums. |

## Running

Build the repository libraries and driver archives first. From `indigo_test/`:

```sh
make build/integration/test_aux_cloudwatcher_simulator build/integration/test_aux_mgbox_simulator
./build/integration/test_aux_cloudwatcher_simulator
./build/integration/test_aux_mgbox_simulator
make test-aux-dragonfly-simulator
```

The two PTY suites belong to `test-integration`. Dragonfly is opt-in because it binds a loopback UDP socket, on an automatically allocated port. Serial/network simulators can also run independently with `--headless --ready-file PATH [--profile NAME] [--trace]`. The ready file follows the existing `INDIGO_SIMULATOR_PORT` contract. Each simulator source is located in its driver's `<driver>_simulator/` directory.

Memory regressions require instrumentation of the driver, not just the harness:

```sh
make build/integration/test_aux_cloudwatcher_simulator_asan build/integration/test_aux_dragonfly_simulator_asan build/integration/test_aux_mgbox_simulator_asan
AUX_TEST_FILTER=timeout ./build/integration/test_aux_cloudwatcher_simulator_asan
AUX_TEST_FILTER=oversized ./build/integration/test_aux_dragonfly_simulator_asan
AUX_TEST_FILTER=short_ ./build/integration/test_aux_mgbox_simulator_asan
```

These targets compile the unchanged driver source with AddressSanitizer and link the existing framework library, which is not instrumented by these targets. `AUX_TEST_FILTER` selects scenario names by substring. Failures remain nonzero; known bugs are not converted to expected passes. Run `make test-clean` after validation.

## Scope and remaining coverage

| Driver | Implemented scenarios | Remaining coverage |
| --- | --- | --- |
| `aux_cloudwatcher` | Lifecycle, firmware 5.89 pressure and wind conversion, relay control and failed acknowledgement, wrong identity, documented low-resolution humidity conversion, silent serial timeout/ASan | Older firmware/optional sensor combinations, precise humidity, binary constants across byte ranges, heater feedback, threshold transitions, reconnection and partial/corrupt block variants. |
| `aux_dragonfly` | Lifecycle, all eight sensors and relays, timed pulse completion, wrong model, failed relay command, oversized UDP response/ASan | Authentication levels, sensor faults, dropped datagrams, simultaneous pulses, renamed channels, reconnection. Full detailed reply catalogue still needs independent manufacturer documentation. |
| `aux_mgbox` | Weather and GPS logical devices in separate lifecycles, pressure/temperature/humidity units, calibration, coordinates/elevation, invalid port, truncated checksummed weather and GPS messages/ASan, disconnect/shutdown | Shared simultaneous connection orders, command tracing for relay duration and reboot/forwarding, bad checksums, fix transitions, reconnection, network transport. The simulator accepts the documented pulse/reboot/forwarding commands, but these are not yet claimed as driver test coverage. |

The scope is the three previously untested portable AUX serial/network drivers. Linux-only `aux_asiair` and `aux_rpio`, and the system-HID `aux_joystick`, are excluded. No previously untested eligible vendor-SDK AUX driver was identified.
