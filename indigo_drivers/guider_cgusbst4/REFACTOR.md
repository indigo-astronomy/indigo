# CG-USB-ST4 guider refactoring and validation record

Status: partial simulator coverage recorded on 2026-09-09. Production refactoring is not complete. `DRV-095` remains open pending protocol confirmation.

## Current-state audit

- The guider is tested through the public INDIGO bus and real PTY I/O against a standalone simulator. Each bounded test case owns and cleans up a fresh simulator process and event log.
- The historical manufacturer page could not be retrieved. [Upstream PHD2 at commit c406cf2b2de51cbb7e3ed76165c9accf33edad2d](https://github.com/OpenPHDGuiding/phd2/blob/c406cf2b2de51cbb7e3ed76165c9accf33edad2d/src/scope_GC_USBST4.cpp) is supplementary evidence: it uses handshake byte `0x06`, reply `A`, numeric directions 0–3 and a four-character millisecond field. INDIGO uses direction letters `n/s/e/w`.
- The simulator exposes explicit `phd2`, `indigo`, `wrong-identity` and `silent` profiles. Its automatic same-axis replacement is a model assumption, not independently verified firmware behavior.
- Current coverage includes all four directions, expiration and public-value reset in the explicit INDIGO dialect, wrong identity, silent identity and reconnect.
- Remaining gaps are authoritative protocol confirmation, overlapping/replaced pulses, transport loss, additional instances and a full timing benchmark.

## Validation and next steps

- The historical test-change log recorded this driver as `Partial` with PTY protocol simulator plus existing fake transport: the PTY run passed 4/5 scenarios; the explicit INDIGO dialect worked, while the numeric PHD2 dialect rejected the emitted commands as `DRV-095`.
- Five ordinary simulator scenarios ran on 2026-09-09: four passed. The PHD2-dialect scenario rejected INDIGO's letter command while INDIGO later reported timer completion, reproducing `DRV-095`.
- Hardware testing was not performed. No hardware protocol compatibility or relay behavior is claimed.
- Before production changes: obtain authoritative protocol evidence or an identified device for hardware testing; record the exact model and hardware plan if available; then plan atomic protocol, overlap, transport-loss, instance and timing work and verify each step through the simulator and applicable hardware.

## Final test summary

- Simulated tests: 5 executed, 4 passed.
- Hardware tests: 0 executed, 0 passed.
