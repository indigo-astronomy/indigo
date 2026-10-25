# Task quality review

Verdict: pass
Reviewer and independence: fresh read-only general-purpose subagent (same model, no implementation context beyond the brief and diff), plus the coordinator's own pass
Reviewed revision and working-tree state: uncommitted diff on the task branch (driver, fake test, Makefile); records reviewed by the coordinator

## Scope and method
Diff of `indigo_drivers/ccd_ptp/indigo_ccd_ptp.c`, `indigo_ptp.h`, `indigo_ptp_sony.c/.h`, `indigo_test/integration/test_ccd_ptp_sdk.c`, `indigo_test/Makefile` with the surrounding exposure state machine, connection handler and fake; style rules from AGENTS.md.

## Quality findings
1. Blocking: confirm window shutter + 3 s would press again during a long exposure noise reduction dark frame (2 × shutter), risking ALERT or a second frame. Fixed: doubled from 1 s, case `sony_long_exposure_noise_reduction`.
2. Non-blocking: retry decided on the cached ObjectInMemory; a late PropertyChanged could cause a spurious second shot. Fixed: decision after `ptp_sony_refresh_properties()`.
3. Non-blocking: SessionAlreadyOpen path closed and reopened immediately. Fixed: `close_session()` / `open_session()` helpers apply the delay there too.
4. Non-blocking, recorded: connection handler may block up to 1 s (bounded, before properties are defined).
5. Non-blocking: fake `stuck_busy` not atomic; elapsed-time assertion satisfied by the Sony init sleep anyway. Fixed: atomic, assertion removed.
6. Non-blocking: obscure double wait. Fixed: `wait_value_long()`.
7. Found by UBSan after the review: fake release after an ignored press used a stale press time. Fixed.
No FND records created; all fixed in this task.

## Fix reinspection
Coordinator reinspected the fixes; negative controls and retests in verification.md.

## Quality verdict
Pass. Remaining limitation: no hardware LENR measurement.
