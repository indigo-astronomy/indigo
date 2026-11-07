# Task quality review

Verdict: pass
Reviewer and independence: fresh general-purpose subagent (same host and model), read-only, independent of the implementation context; one pass over the final code diff; the R1 documentation correction was applied and reinspected by the implementer
Reviewed revision and working-tree state: master 0d1e4672e plus the uncommitted task diff on task/20261107080809-a02b-fix-ccd-svb-gain-after-open (2026-11-07)

## Scope and method
Static review of `git diff master -- indigo_drivers/ccd_svb indigo_test MIGRATION_STATUS.md` (README.md and
TEST_SUMMARY.md records excluded), the generated connection handler of `indigo_ccd_svb.c` against the `.driver`
diff, `svb_setup_exposure()`, `acquisition_start()` and the countdown path, the guider device's shared session,
the fake SDK's open/control-write code and `begin_fixture()`, every existing case touching `SVB_GAIN` or
`write_config_fail`, the hardware suite's workflow sequence, the 3.0-7 driver (`git show cd408e64b:...`, lines
1111-1113) and the INDI driver (`svbony_base.cpp` line 648), the root AGENTS.md formatting/C/generated-driver
rules, `indigo_test/AGENTS.md` and the quality-review routine. Whitespace checked mechanically. The reviewer did
not run the generator, the build or the tests; the run evidence is in verification.md.

## Quality findings
| ID | Severity | Location | Finding | Disposition |
| --- | --- | --- | --- | --- |
| R1 | informational | specification.md | two statements were inaccurate: 3.0-7 wrote the exposure before the control enumeration (so it showed 1 s at connect, the new order shows the SDK's pre-write value), and the hardware step was described as compiled only although the SV305PRO runs exist | fixed: both statements corrected in the specification; no code change |
| R2 | informational | `test_ccd_svb_sdk.c` `accepts_gain_before_exposure` | the quirk is active by default, while `indigo_test/AGENTS.md` prefers the well-behaved default when only some models misbehave | deliberate: the refusal is the SDK behaviour on every model tested (SV305PRO, SV705C, SV905C2, SV605CC); existing cases keep their meaning and the two that now depend on the connect write are the user's scenario; recorded here |
| R3 | style | `.driver` connect log vs `svb_set_number()` log | the control is printed symbolically in one line and numerically in the other | no change |
| R4 | style | `test_ccd_svb_hw.c` `gain_before_first_exposure()` | a missing or read-only `CCD_GAIN` passes silently without a SKIP line, like the pre-existing gain block of `format_geometry_and_controls()` | no change; matches the file's convention |

No high, medium or low findings; no confirmed defects, no formatting-rule violations, no scope drift.
Pre-existing issues outside the diff noted by the reviewer, not changed by this task: `init_camera_property()`
prints a `long` with `%d` in the `SVB_GAIN` debug line and has two mis-indented `return INDIGO_FAILED;` lines.

The reviewer confirmed: every hunk of the regenerated `.c` corresponds to a `.driver` hunk and the version define
matches `version = 35`; the connect-time write is the same call, value and auto flag as 3.0-7 and INDI, runs only
after a successful `initialize_camera()` inside the generator's `on_connect` block without a `return`, under the
master lock the block already holds, and leaves the generated rollback untouched; the write happens once per CCD
connect and is harmless when the guider holds the session; `svb_setup_exposure()` rewrites the requested duration
for every acquisition and the countdown never reads the SDK value; the log format specifiers match the argument
types and `value` is initialised for the failed-readback line; the fake SDK clears the state on open, sets it only
on a successful exposure write and refuses the gain only while clear and not opted out; no existing case connects
with a forced exposure-write failure; `starts` is incremented only by `SVBStartExposure()`/`SVBSendSoftTrigger()`,
so `starts == 0` proves no exposure; the `MIGRATION_STATUS.md` count 49 matches the registered cases minus the
`--timing` entry; the hardware step runs before the first exposure of the run and leaves the abort-latency and
hot-plug paths untouched; formatting rules are met.

## Fix reinspection
R1: specification wording corrected (failure-behaviour bullet, hardware-suite bullet, AC-007 method, owner
decisions); reinspected by the implementer only (same-agent limitation).

## Quality verdict
Pass. Remaining limitations: static review only (run evidence in verification.md); hardware evidence for the
SV305PRO only.
