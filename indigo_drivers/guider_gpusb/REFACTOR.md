# GPUSB guider refactoring and validation record

Status: partial fake-SDK coverage recorded on 2026-09-09. Production refactoring is not declared complete by this record.

## Current-state audit

- The generated guider uses a USB/SDK boundary. Hardware-free validation uses the fake USB/SDK implementation in `indigo_test/integration/test_usb_outputs.c`; no serial protocol is invented for this USB-only device.
- The four existing groups cover discovery, duplicate events, connection rollback, active removal and reload; direction mapping, delayed completion and errors; initialization and attach rollback; settings, abort, stop/read errors and removal during operation.
- Direction coverage includes all four directions, cross-axis pulses, reversal and zero stop. Regression coverage includes failed-replacement preservation and single-write zero-stop behavior.
- Remaining gaps recorded for this validation are the detailed timing and capacity audit.

## Validation and next steps

- The historical test-change log recorded this driver as `Partial` with fake USB/SDK coverage: lifecycle failures, four directions, cross-axis pulses, reversal, zero stop, active removal, failed-replacement preservation and single-write zero-stop regressions pass; timing and capacity audit remain pending.
- All four fake-SDK test groups passed on 2026-09-09.
- Hardware testing was not performed. No physical relay, USB timing or hardware lifecycle validation is claimed.
- Before further production refactoring: reconfirm the fake-SDK baseline; audit current coverage against the Guider Driver Test Standard; plan atomic timing, capacity, sanitizer and remaining lifecycle/concurrency work; verify and record each step before claiming completion.

## Final test summary

- Simulated/fake-SDK tests: 4 executed, 4 passed.
- Hardware tests: 0 executed, 0 passed.
