# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001, AC-002 | integration, `indigo_test/integration/test_ccd_qhy_sdk.cpp` case `slow readout timeout` (qhy2 build) | Fake SDK, readout 1 s, RAW, exposures 0.1 s and 1 s | 2 blobs with the exact SDK payload, 0 bad payloads | Before and after the fix |
| T-002 | AC-003 | same case against the unfixed driver | as T-001 | both frames are zeros, 2 bad payloads | Before the fix |
| T-003 | AC-004 | full `test_ccd_qhy2_sdk` | Fake SDK | all cases pass | After the fix |
| T-004 | AC-004 | full `test_ccd_qhy_sdk` (legacy driver, which never sets the timeout; the new case is QHY2 only) | Fake SDK | all cases pass | After the fix |

## Baseline evidence
See verification.md.

## Bug reproduction
The fake SDK models the reported symptom: a frame that exceeds the timeout returns success with a buffer of zeros. Against the unfixed driver both frames of the new case (0.1 s and 1 s, each with a 1 s readout) are published as zeros and the case fails with 2 bad payloads.

## Negative cases and permission boundaries
Existing cases already cover read failures (`fail_read`), deadline handling and read-directly timing. The timeout model leaves `fail_read` unchanged.

## Isolation and cleanup
`end()` resets `readout_time` to 0 and `frame_timeout` to the SDK default 60000. The case disconnects and restores `verify_raw`.

## Deferred tests and coverage limitations
- No hardware run. The real SDK's timeout behaviour and the QHY168C readout time are not measured.
- No `tools/run_driver_test.py` recorded run was requested, so no README test record is written.
