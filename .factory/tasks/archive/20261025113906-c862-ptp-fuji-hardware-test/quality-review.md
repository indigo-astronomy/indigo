# Quality review

Verdict: pass
Note: after the fixes below
Reviewed revision: `e49bfce27` plus the uncommitted task diff (driver, fake suite, hardware suite, records)
Reviewer: fresh-context general-purpose subagent (read-only), same host and model as the implementer; resolutions by the implementer
Date: 2026-10-25

## Findings and resolutions
| # | Severity | Finding | Resolution |
| --- | --- | --- | --- |
| 1 | low | `set_property_complete()` could dereference a property the refresh removed | fixed: NULL guard, pending request still released |
| 2 | low | Sony refreshed twice on the finalizer timeout path | fixed: readback only on the write failure paths (`read_back_rejected()`) |
| 3 | low | a transport failure costs one extra readback transaction | accepted: bounded by the transaction timeout, transport errors are not distinguishable from GeneralError |
| 4 | low | readback not covered for Canon/Nikon/Olympus/Sony | accepted: existing `property_error_recovery` and `sony_dropped_setting_write` still pass; follow-up with the Sony finding |
| 5 | low | `sony_charger_not_attached` used a fixed sleep; `rejected_setting_readback` read driver state racily | fixed: ordered by a following removal event; internal read removed |
| 6 | note | `has_ptp_interface` repeats the `ptp_open()` interface test | accepted: kept local to the plug path |

Later changes after the review (suite restore and abort timing, immediate Bulb removal after a shutter write) were checked by the hardware and fake runs in verification.md; the immediate removal has the strengthened `fuji_bulb_not_offered` regression.

## Retest
Full fake (normal and sanitizer) and recorded hardware runs after the fixes; see verification.md.
