# AUX CloudWatcher refactoring and validation record

Status: partial simulator coverage recorded on 2026-09-09. Production refactoring is not complete. `DRV-085` and `DRV-086` were fixed on 2026-09-18 and the whole simulator suite now passes, including under ASan.

## Current-state audit

- The hand-written AUX driver communicates over a serial protocol and is tested through the public INDIGO bus against a standalone PTY simulator.
- Manufacturer sources are the [Lunático protocol v1.4](https://lunaticoastro.com/rs232-communication-protocol-cloudwatcher/), [v1.3](https://lunaticoastro.com/aagcw/TechInfo/Rs232_Comms_v130.pdf), [v1.1 binary constants](https://lunaticoastro.com/aagcw/TechInfo/Rs232_Comms_v110.pdf) and [v1.0 framing](https://lunaticoastro.com/aagcw/TechInfo/Rs232_Comms_v100.pdf). Existing driver behavior is supplementary evidence only.
- The simulator models firmware 5.89 with SQ, humidity and pressure sensors. Tests cover lifecycle, pressure and wind conversion, relay control and failed acknowledgement, wrong identity, low-resolution humidity conversion and silent serial timeout.
- Known gaps are older firmware and optional sensor combinations, precise humidity, binary constants across byte ranges, heater feedback, threshold transitions, reconnect and additional partial/corrupt block variants.
- `DRV-085` (fixed 2026-09-18, driver version `0x0200000A`): low-resolution humidity used `(rhi * 1.7572) / 100 - 6`, a copy of the temperature coefficient. The precise branch scales the 16-bit reading by `125 / 65536`, so a reading expressed as a percentage of full scale scales by `125 / 100`, exactly as the low-resolution temperature branch uses `175.72 / 100`. With the simulator's `!h 40` the driver now reports 44 %RH instead of -5.3 %RH. The precise branch also used integer division (`(rhi * 125) / 65536`), quantising humidity to whole percent; it now uses `125.0`.
- `DRV-086` (fixed 2026-09-18, same version): a timeout before the first response byte left `index` at 0 and executed `response[index - 1] = 0`, writing one byte in front of the caller's buffer. The index is now clamped. The driver-instrumented ASan run of the `timeout` scenario reproduced the underflow before the fix and is clean after it.

## Validation and next steps

- The historical test-change log recorded this driver as `Partial` with a PTY protocol simulator: 4/5 ordinary scenarios passed, documented humidity conversion failed as `DRV-085`, and ASan exposed the timeout buffer underflow `DRV-086`.
- Baseline macOS arm64/x86_64 build completed on 2026-09-09. Five ordinary simulator scenarios ran: four passed and the documented humidity conversion scenario failed.
- On 2026-09-18 both defects were fixed and re-verified on macOS arm64/x86_64: all five ordinary simulator scenarios pass, and the driver-instrumented ASan build passes all five with no sanitizer reports. The framework library is still not instrumented.
- The test only relinked against a rebuilt driver after `indigo_aux_cloudwatcher.a` was added as a prerequisite of its `Makefile` rule; before that the suite silently exercised a stale archive, which is why `DRV-085` appeared to pass.
- Hardware testing was not performed. No hardware validation is claimed.
- Remaining work is unchanged for the coverage gaps listed above: older firmware and optional sensor combinations, precise humidity, binary constants across byte ranges, heater feedback, threshold transitions, reconnect and additional partial/corrupt block variants.

## Final test summary

- Simulated tests: 10 executed, 10 passed (5 ordinary: 5 passed; 5 ASan: 5 passed).
- Hardware tests: 0 executed, 0 passed.
