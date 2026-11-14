# Verification report

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5)
Date: 2026-11-14
Revision and working-tree state: branch `scheduler` at 7bd0c2e5e with this task's uncommitted changes
Review independence: not independent; covered by mutation runs, a defect reproduced before and passing after the fix, and the full suites.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | nine eligibility cases: reasons "target below the altitude limit", "airmass too high", "hour angle out of range", "too bright (twilight)", "too close to the moon", "moon too bright", "outside of the nightly window", "nightly window ends before the task can finish", "target below the horizon" (duration), "site coordinates unknown"; positive tasks DONE; Steep, Thin air, East, Night only, Placed and Dark only start after the site moves or appears; Low sides (3 .. -3) stays waiting; Moonless runs with the moon below the horizon; @enabled no not overridden by ENABLE, enabled by an edit |
| AC-002 | pass | sky_bonuses_order_equal_priorities: Setting before Rising (level 50 vs 70); with SETTING_WEIGHT 0 Rising first; with both weights 0 the list order (Setting moved up); with ALTITUDE_WEIGHT 2 the 75-degree Rising first |
| AC-003 | pass | 16384-byte task stored, restored with all bytes and run; 16385 rejected "longer than 16384 bytes"; 128 tasks, 129th "at most 128 tasks", urgent "too many tasks"; delete frees a place |
| AC-004 | pass | second starts 55-80 s after the end of the first run and after the failure |
| AC-005 | pass | device cases 9-11: 2 frames per L/R/G/B with FILTER, IMAGETYP Light, EXPTIME 0.2 and OBJCTRA/OBJCTDEC at the target; Dark 0.3 s; Bias (simulator records 0.1 s); wheel at slot 4; West (high) before East (normal), pointing per frame, East DONE run 2/2 with 2 frames; Bad filter started twice then FAILED, Bad camera once, messages name the missing filter and camera, no frames, Afterwards captured |
| AC-006 | pass | targets_in_priority_order failed twice before the fix and passes after it; start_waits_for_pending_estimates passes (guard) |
| AC-007 | pass | SCHEDULER.md, guide reason line, version 0x03000009, MIGRATION_STATUS 71 / 0, FND-20261114095750-d502-scheduler-eligibility-and-device-tests-001 |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make -C indigo_drivers/agent_scheduler -f ../../Makefile.drv all` | repository root, Linux x86_64 | 0 | no warnings |
| sky mutations (altitude limit off, wrapped hour angle inverted, moon checks always, crossing nightly window off, interval checks only the start), with the driver rebuilt | indigo_test | 1 each | each makes its case fail; sources restored (cmp) |
| mutations: setting term removed; retry delay 0; repeat pause 0 | indigo_test | 1 each | sky_bonuses / repeat_interval cases fail; restored |
| `run_scheduler_simulators.py` before the fix | repository root | 1 | 9/11 twice: targets_in_priority_order (defect 1), and an over-strict bias EXPTIME expectation (test corrected: the simulator records 0 s as 0.1 s) |
| `run_scheduler_simulators.py` after the fix | repository root | 0 | 11/11 |
| `test_agent_scheduler`, `test_agent_scheduler_asan` | indigo_test | 0 | 71/71 each, no sanitizer report |
| `python3 tools/run_driver_test.py agent_scheduler` | repository root | 0 | 82/82 OK (71 integration + 11 server acceptance), recorded 3.0.0.9 |
| `./.factory/scripts/findings check` | repository root | 0 | valid, 34 active |

## Baseline comparison and regressions
Baseline 65/65 at 3.0.0.8; no regressions.

## Found defects
| Defect | Impact | Cause | Fix | Regression test | Found by |
| --- | --- | --- | --- | --- | --- |
| Lower-priority task started while a higher one waited for its estimate | priority inversion when START follows uploads | select_task() skipped estimate-pending tasks and started the best of the rest | no ordinary task starts while a selectable task waits for its estimate; urgent tasks still start (owner approved) | device targets_in_priority_order; guard start_waits_for_pending_estimates | new device case |
| Trailing whitespace of a stored task lost across a restart | downloaded text differs from the uploaded one | configuration reader trims trailing whitespace | not fixed, owner chose a finding: FND-20261114095750-d502-scheduler-eligibility-and-device-tests-001 | largest_task_text_is_stored_and_restored ends with a non-whitespace byte | new integration case |
| Test defects during development | false results | wheel property redefined (None), delays measured after another wait, bias minimum | tests corrected | | own runs |

## Manual checks and limitations
The in-process guard cannot reproduce defect 1 because the estimate script runs on the scheduler's own handler queue there. Hardware not run.

## Verification verdict rationale
All criteria pass on the final candidate.

## Final test summary
- Simulated tests: recorded run 82 run, 82 passed; ASan 71 run, 71 passed.
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
One open finding, low, non-blocking: FND-20261114095750-d502-scheduler-eligibility-and-device-tests-001.
