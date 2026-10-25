# Quality review

Verdict: pass
Note: after the fixes below
Reviewed revision: `a89afe87d` plus the uncommitted task diff (driver, fake suite, new hardware suite, Makefile)
Reviewer: fresh-context general-purpose subagent (read-only), same host and model as the implementer; resolutions by the implementer
Date: 2026-10-25

## Findings and resolutions
| # | Severity | Finding | Resolution |
| --- | --- | --- | --- |
| 1 | medium | the hardware suite promised no card deletion, but the driver default deletes downloaded frames on Canon/Nikon/Olympus | fixed: DSLR_DELETE_IMAGE switched OFF on connect |
| 2 | medium | settings not remembered before the identity checks; not restored when a case ended disconnected | fixed: remembered right after connect; exit path reconnects and restores, failure to restore fails the run |
| 3 | low | dropped-write fake case with two dropped writes had little margin in the 3 s window | fixed: one dropped write; never-applied case kept |
| 4 | low | `strtoll`/`strtol` overflow for negative enum item names | fixed: `strtoull` in verification and in the Sony setter |
| 5 | note | ALERT after a failed final refresh shows the requested items; switch flicker while BUSY | accepted: final state is the camera's on the normal path |

Finalizer lifetime, queue serialization, DeviceBusy interaction, union aliasing, excluded `+`/`-` items, other vendors and Sony old API found clean.

## Retest
Full fake (normal and sanitizer) and recorded hardware runs after the fixes; see verification.md.
