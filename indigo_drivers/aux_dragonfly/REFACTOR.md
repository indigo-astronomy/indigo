# AUX Dragonfly refactoring and validation record

Status: partial simulator coverage recorded on 2026-09-09. Production refactoring is not complete. `DRV-087` remains open.

## Current-state audit

- The hand-written AUX driver communicates over UDP and is tested through the public INDIGO bus against a separately launched loopback simulator. The suite is opt-in because it binds a loopback UDP socket.
- Primary public sources are the [Seletek developer guide](https://lunaticoastro.com/seletek-developers-guide/) for SLP framing, UDP and channel ranges and the [Dragonfly JavaScript API](https://lunaticoastro.com/df-javascript/) for pulse units and behavior. `relio_simulator/relio_simulator.pl` supplies supplementary reply forms because the public guide defers the complete command catalogue to a spreadsheet available on request.
- Ordinary tests cover lifecycle, all eight sensors and relays, timed pulse completion, wrong model and failed relay command.
- Known gaps are authentication levels, sensor faults, dropped datagrams, simultaneous pulses, renamed channels, reconnect and independently documented detailed reply forms.
- `DRV-087`: a full-size UDP reply writes the terminating NUL beyond the response buffer; the driver-instrumented ASan case reproduces it.

## Validation and next steps

- The historical test-change log recorded this driver as `Partial` with an opt-in UDP protocol simulator: all 5 ordinary scenarios passed, including eight sensors/relays and a timed pulse, and ASan exposed the oversized-reply overflow `DRV-087`.
- Baseline macOS arm64/x86_64 build completed on 2026-09-09. All five ordinary simulator scenarios passed.
- One selected driver-instrumented ASan simulator scenario ran and failed with the `DRV-087` stack-buffer-overflow. The framework library was not instrumented.
- Hardware testing was not performed. No hardware validation is claimed.
- Before production changes: reconfirm the baseline; plan and implement an atomic bounds fix for `DRV-087`; extend the simulator for the listed gaps; run the complete ordinary, strict-warning and sanitizer suites; then record each verified step and reconcile review/test status.

## Final test summary

- Simulated tests: 6 executed, 5 passed (5 ordinary: 5 passed; 1 ASan: 0 passed).
- Hardware tests: 0 executed, 0 passed.
