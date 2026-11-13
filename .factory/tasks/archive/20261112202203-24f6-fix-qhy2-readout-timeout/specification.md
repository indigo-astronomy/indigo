# Specification

Status: approved by the owner's instruction "ano, oprav to" (2026-11-12)

## Problem and outcome
`indigo_ccd_qhy2` limits every single frame to 1000 ms with `SetQHYCCDSingleFrameTimeOut()`. The SDK applies this limit to the exposure and the readout together. A camera whose exposure plus readout exceeds it returns a frame of zeros (indigo#783). The driver must give the SDK a limit that covers the requested exposure and a readout allowance.

## In scope
The single-frame timeout set in `qhy2_setup()`, the driver version, and the fake-SDK model and regression test.

## Out of scope
See brief.md non-goals.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | For a single exposure of duration d seconds, the driver sets the SDK single-frame timeout to ceil(d × 1000) + 10000 ms, capped at `UINT32_MAX`. This is the same 10 s readout allowance as the driver's acquisition deadline (`started + duration + 10`). | Fake SDK records the timeout; `slow readout timeout` case prints and asserts it |
| AC-002 | With a 1 s readout, exposures of 0.1 s and 1 s publish an image whose payload equals the SDK frame, not an empty buffer. | `slow readout timeout` case, `verify_raw` payload check, 2 blobs and 0 bad payloads |
| AC-003 | The same test fails against the unfixed driver (1000 ms). | Baseline run before the driver change |
| AC-004 | Existing qhy2 and legacy qhy fake-SDK behaviour is preserved. | Full `test_ccd_qhy2_sdk` and `test_ccd_qhy_sdk` runs pass |
| AC-005 | Driver version raised from 46 (0x0300002E) to 47 (0x0300002F); the `.cpp` is regenerated from the `.driver`. | Diff inspection, build |

## Failure behavior and edge cases
- A camera that never delivers a frame is still bounded: the SDK gives up after exposure + 10 s. This matches the acquisition deadline, so the device queue is never blocked indefinitely; the SDK limit is not set to 0 (unlimited).
- Read-directly cameras: the read blocks for the exposure, which the new limit includes.
- Streaming does not call `SetQHYCCDSingleFrameTimeOut()` and is unchanged.
- Overflow: the maximum `CCD_EXPOSURE` is `fmax(SDK maximum / 1e6, 900)` s, so it depends on the SDK. The value is therefore capped at `UINT32_MAX` before the cast.

## Permissions, data, and security requirements
No change to data handling or trust boundaries.

## Compatibility and nonfunctional requirements
Portable C++ (`ceil`, `uint32_t`), no platform code. Faster cameras are unaffected, because the SDK returns as soon as the frame arrives.

## Assumptions and open questions
- Assumption (low risk, not blocking): the real SDK returns success with an unfilled buffer (all zeros in the report) when the single-frame timeout elapses. This is inferred from the report and not confirmed. If the QHY168C still returns zeros after this fix, the next suspect is the effective-area ROI offset (EZCAP reads 5040×3346, INDIGO 4952×3288 with an offset).
- Hardware confirmation depends on the reporter testing a build.

## Owner decisions and requirement changes
2026-11-12, Peter Polakovic: "ano, oprav to" — fix the readout timeout as proposed (exposure plus readout allowance).
