# Quality review

Verdict: pass
Note: after the fixes below
Reviewed revision: base 93bb23d76 plus the uncommitted task diff (`indigo_drivers/ccd_ptp/`, `indigo_test/integration/test_ccd_ptp_sdk.c`)
Reviewer: fresh-context general-purpose subagent (read-only), same host and model as the implementer; resolutions by the implementer
Date: 2026-10-25

## Findings and resolutions
| # | Severity | Finding | Resolution |
| --- | --- | --- | --- |
| 1 | medium | Fuji download leaked the transport buffer when GetObject returned an empty object (new `&& image_size` check) | fixed: buffer freed in the else branch |
| 2 | low | CustomFuncEx fake resent its record on every poll, so the polling thread rewrote the groups the test reads | fixed: record sent once per profile |
| 3 | low | `device_info_exact_buffer` reset its fault knob only at the end | fixed: knob reset right after building the reply |
| 4 | note | Sony GetAllDevicePropData, `ptp_decode_property_value()` and Canon image-format lists still read without a reply length | registered as FND-20261025093224-2d50-fix-ptp-findings-001 |

The reviewer confirmed the bounded readers, CHECK_SIZE sizes, enum clamps, ObjectInfo offsets, CustomFuncEx validation/storing/`set_number_property` consistency, string termination, capacity clamps and ICA compatibility, and that well-formed replies decode as before.

## Retest
Full sanitizer and recorded runs after the fixes; see [verification.md](verification.md).
