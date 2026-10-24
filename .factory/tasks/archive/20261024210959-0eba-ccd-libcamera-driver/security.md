# Security review

Verdict: pass
Date: 2026-10-24. Reviewed state: working-tree candidate over `a3d9a78fc`; source identities in process.json. Same agent as the implementation; the independent quality reviewer also inspected buffer handling (quality-review.md).

## Boundaries and coverage
The driver talks only to the local libcamera library; no network endpoint, authentication, permission, credential, persistent configuration or file path input is added. Data crossing the trust boundary is the frame buffer and metadata from libcamera: `lcam_read()` checks the requested rectangle against the configured frame size and the destination size, checks stride against the row size and the plane length before reading the mapped buffer, and copies whole rows only within `row_bytes`. The driver checks the returned geometry against `CCD_FRAME` before publishing and allocates the blob buffer from the largest mode with an overflow check. Request completion crosses threads only through a mutex-protected queue; buffers are unmapped after `Camera::stop()` cancels pending requests. Generator change: no change for existing definitions (byte-identical output); the new mode removes libusb usage.

## Checks and findings
`./.factory/scripts/security` exit 0 (Factory process boundaries only). ASan/UBSan fake-bridge run clean for the driver object. No security finding, no risk acceptance. The C++ bridge is not sanitizer-instrumented on hardware.

## Data-model path signal
The conservative data-model rule matches `MIGRATION_STATUS.md` and `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`. The first gains one new row for `ccd_libcamera` (status columns only, empty Comment column, existing rows unchanged); the second gains a documentation section for the `usb = false` SDK mode. No schema, stored user data, configuration format or data migration changes. This report covers that signal.
