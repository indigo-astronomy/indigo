# Verification report

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5); independent read-only reviewer subagent (fresh context, same model)
Date: 2026-11-14
Revision and working-tree state: branch `scheduler` at 2646b7afa with this task's uncommitted changes
Review independence: the reviewer received the diff and SCHEDULER.md, not the writer's conclusions.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | unit night: ends_lie_on_the_limit, limits_nest, offsets_move_the_ends, offsets_leave_no_time, offsets_merge_nights, no_night_and_polar_night, lookup |
| AC-002 | pass | unit night: once_per_night_over_three_nights, start_during_night_and_day, boundaries_passing_during_a_task, recomputed_and_moved_nights, real_nights_over_three_days; integration night_hooks_run_at_dusk_and_dawn, stop_at_night_runs_only_the_shutdown_hook |
| AC-003 | pass | unit night_without_work, work_appears_late, night_loses_its_work; integration night_without_work_skips_the_hooks, hooks_named_during_a_night_without_work; server acceptance night case |
| AC-004 | pass | integration resume_hook_after_idle, resume_before_an_urgent_task, idle_without_resume_behaves_as_before, night_hooks_run_at_dusk_and_dawn (IDLE before night, after task, not after NIGHT_END), failed_night_end_is_reported |
| AC-005 | pass | integration failed_night_start_runs_the_shutdown_hook, failed_resume_runs_the_shutdown_hook |
| AC-006 | pass | integration shared_hook_task_runs_once (with positive control), night_end_and_shutdown_share_a_task, missing_night_hook_task_is_reported |
| AC-007 | pass | unit plan hooks_reserve_time; integration night_hooks_run_at_dusk_and_dawn, night_without_work_skips_the_hooks, plan_lasts_until_a_late_night_end, no_night_is_reported |
| AC-008 | pass | integration persistence_across_restart |
| AC-009 | pass | the 36 earlier integration cases unchanged and passing; idle_without_resume_behaves_as_before; night_without_work_and_without_hooks_ends_quietly |
| AC-010 | pass | server acceptance "night hooks prepare and put away the devices": CCD_COOLER ON with target 20 °C and mount UNPARKED after NIGHT_START, CCD_COOLER OFF and PARKED after NIGHT_END |
| AC-011 | pass | names, PROPERTIES.md, SCHEDULER.md, user guide, Xcode (14 entries), Visual Studio project and filters, test Makefile, version 0x03000006, MIGRATION_STATUS 52 / 0 |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| baseline: driver build, unit header/sky/plan, `test_agent_scheduler` | repository root, Linux, 2646b7afa | 0 | build OK, unit pass, 36/36 |
| `make -C indigo_drivers/agent_scheduler -f ../../Makefile.drv all` | repository root | 0 | no errors or warnings |
| `gcc -O2 -D_FORTIFY_SOURCE=2 -Wall -Wextra` on agent, night and plan sources | repository root | 0 | no new warnings after initialising the Sun arrays |
| unit `test_agent_scheduler_header`, `_sky`, `_plan` (15), `_night` (16) | indigo_test | 0 | all passed |
| mutation: NIGHT_END after a skipped night | indigo_test | — | unit night_without_work failed as expected; original restored |
| mutation: late skip notice reset removed | indigo_test | — | hooks_named_during_a_night_without_work failed as expected; original restored |
| `build/integration/test_agent_scheduler` | indigo_test | 0 | 52/52 (earlier full runs 46/46 twice before the review cases) |
| `ASAN_OPTIONS=detect_leaks=1 build/integration/test_agent_scheduler_asan` | indigo_test | 0 | 52/52 on the final candidate, no sanitizer report |
| `python3 tools/run_driver_test.py agent_scheduler` | repository root, Linux x86_64 | 0 | 60/60 OK (52 integration + 8 server acceptance), recorded in README.md and TEST_SUMMARY.md as 3.0.0.6 |
| `./.factory/scripts/findings check` | repository root | 0 | `Findings valid: 30 active, 296 archived; task evidence checks passed.` |
| anchor and link check of SCHEDULER.md and the guide; `git diff --check` | repository root | 0 | no broken link; only the CRLF endings of the Visual Studio files, which are CRLF throughout |

## Baseline comparison and regressions
The 36 baseline integration cases pass unchanged; the persistence case gained assertions.

## Found defects
| Defect | Impact | Cause | Fix | Regression test | Found by |
| --- | --- | --- | --- | --- | --- |
| Idle period restarted at night boundaries without a named night hook | IDLE would run again at dawn and dusk in configurations without night hooks | reset in process_night for every boundary | reset only when a night hook starts (start_task) | idle_without_resume_behaves_as_before, night_without_work_and_without_hooks_ends_quietly | source audit by the writer |
| Shared NIGHT_END and SHUTDOWN task ran twice on STOP during the night end hook | park and close sequence run twice | stop branch did not check the hook chain | shutdown skipped when the same task has just run successfully | night_end_and_shutdown_share_a_task | reviewer |
| Night computation used a static buffer | wrong nights if the module is used from two threads | static array in an exported function | buffer allocated per call | unit night suite | reviewer |
| Notice and skip message lost when a night hook is named after START | the user is not told why nothing was prepared | state marked as reported while the message was suppressed | reported only when sent | hooks_named_during_a_night_without_work (reproduced failing without the fix) | reviewer (notice), reproduced (skip) |
| "Has work" decided from a plan up to one check interval old | skip message or a late NIGHT_START right after an upload | plan updated after the decision | plan recomputed before the night decision when dirty | night_without_work_skips_the_hooks | reviewer |
| No-night Alert item before the task Alert items | differs from the specification | order in update_plan | appended last | no_night_is_reported | reviewer |

Test-only defects fixed during the work (no product change): persistence case named its own tasks as hooks; server case lacked a task to keep the scheduler running and named the hooks after START; plan case read a stale plan; server cooling target needed 3 minutes; shared-hook case lacked a positive control.

## Manual checks and limitations
- The integration cases use the wall clock; dusk and dawn are produced by moving the stand-in site longitude with the agent's own night computation (design deviation: no settable clock in production code, documented in SCHEDULER.md "Testing").
- Not covered by an integration case: site unknown or changed during a handled night (unit recomputed_and_moved_nights covers the decision); a task spanning a night end and the next start (unit only).
- Hardware testing: not performed; no hardware validation is claimed. Only Linux was available; macOS and Windows builds not run (project files updated).

## Verification verdict rationale
All acceptance criteria pass on the final candidate; every product defect found has a regression test.

## Final test summary
- Simulated tests: recorded run 60 run, 60 passed (52 integration, 8 server acceptance with the CCD and mount simulators); ASan/UBSan 52 run, 52 passed; unit suites header, sky, plan (15) and night (16) passed.
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
No findings opened; all defects found were fixed in this task.
