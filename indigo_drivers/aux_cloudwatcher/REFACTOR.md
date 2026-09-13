# AUX CloudWatcher refactoring and validation record

Status: partial simulator coverage recorded on 2026-09-09. Production refactoring is not complete. `DRV-085` and `DRV-086` remain open.

## Current-state audit

- The hand-written AUX driver communicates over a serial protocol and is tested through the public INDIGO bus against a standalone PTY simulator.
- Manufacturer sources are the [Lunático protocol v1.4](https://lunaticoastro.com/rs232-communication-protocol-cloudwatcher/), [v1.3](https://lunaticoastro.com/aagcw/TechInfo/Rs232_Comms_v130.pdf), [v1.1 binary constants](https://lunaticoastro.com/aagcw/TechInfo/Rs232_Comms_v110.pdf) and [v1.0 framing](https://lunaticoastro.com/aagcw/TechInfo/Rs232_Comms_v100.pdf). Existing driver behavior is supplementary evidence only.
- The simulator models firmware 5.89 with SQ, humidity and pressure sensors. Tests cover lifecycle, pressure and wind conversion, relay control and failed acknowledgement, wrong identity, low-resolution humidity conversion and silent serial timeout.
- Known gaps are older firmware and optional sensor combinations, precise humidity, binary constants across byte ranges, heater feedback, threshold transitions, reconnect and additional partial/corrupt block variants.
- `DRV-085`: low-resolution humidity uses the wrong coefficient. `DRV-086`: a timeout before the first byte writes before the response buffer; the driver-instrumented ASan case reproduces it.

## Validation and next steps

- The historical test-change log recorded this driver as `Partial` with a PTY protocol simulator: 4/5 ordinary scenarios passed, documented humidity conversion failed as `DRV-085`, and ASan exposed the timeout buffer underflow `DRV-086`.
- Baseline macOS arm64/x86_64 build completed on 2026-09-09. Five ordinary simulator scenarios ran: four passed and the documented humidity conversion scenario failed.
- One selected driver-instrumented ASan simulator scenario ran and failed with the `DRV-086` stack-buffer-underflow. The framework library was not instrumented.
- Hardware testing was not performed. No hardware validation is claimed.
- Before production changes: reconfirm the baseline; plan and implement atomic fixes for `DRV-085` and `DRV-086`; extend the simulator for the listed gaps; run the complete ordinary, strict-warning and sanitizer suites; then record each verified step and reconcile review/test status.

## Final test summary

- Simulated tests: 6 executed, 4 passed (5 ordinary: 4 passed; 1 ASan: 0 passed).
- Hardware tests: 0 executed, 0 passed.
