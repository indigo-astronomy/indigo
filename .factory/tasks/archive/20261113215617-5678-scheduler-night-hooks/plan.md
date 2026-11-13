# Development plan

Status: complete

## Approach and affected components
New module `indigo_agent_scheduler_night.c/.h` (Sun sampling, nights, pure decision function); `indigo_scheduler_plan_with_hooks()` in the plan module; agent property, settings, hook items, standby, RESUME, chaining of shared hook tasks, failure handling, plan items, persistence; tests in three layers; docs and project files.

## Baseline
- Branch `scheduler` at 2646b7afa. Build OK; unit header/sky/plan pass; integration 36/36 (verification.md).

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Baseline build and tests | — | None | 36/36 | done |
| 2 | Night module and unit tests | AC-001..AC-003 | 1 | unit night 16/16, mutation caught | done |
| 3 | Plan simulation with hooks and unit case | AC-007 | 1 | unit plan 15/15 | done |
| 4 | Names in indigo_names.h; agent changes, version 0x03000006 | AC-002..AC-008 | 2, 3 | build without warnings; 36/36 unchanged | done; found and fixed: idle period reset at unnamed night boundaries (verification.md) |
| 5 | Integration cases (16 new, persistence extended) | AC-002..AC-009 | 4 | full suite, ASan/UBSan | done: 52/52; test setup errors fixed (verification.md) |
| 6 | Server acceptance night case | AC-010 | 4 | run_scheduler_simulators.py | done: 8/8 after two test setup fixes |
| 7 | PROPERTIES.md, SCHEDULER.md, user guide | AC-011 | 4 | link check | done |
| 8 | Xcode, Visual Studio, test Makefile, MIGRATION_STATUS 52/0 | AC-011 | 2 | review | done |
| 9 | Independent review, fixes, recorded run | all | 5-8 | verification.md | done: six product defects fixed with tests; recorded run 60/60 |

## Test strategy
Pure decision function tested over synthetic and real nights; plan with hooks tested directly; integration cases move the stand-in site longitude so that dusk or dawn is seconds away (the agent uses the wall clock; no test clock was added to production code); server acceptance moves the Mount Agent site between equator midnight and noon.

## Security risks and mitigations
Hook names resolved by lookup; fixed arrays (`SCHEDULER_NIGHT_MAX` 8, two plan hooks); offsets clamped.

## Dependencies and architecture decisions
No new dependency. Design deviation (settable clock replaced by site moves) documented in SCHEDULER.md "Testing".

## Migration, rollout, and rollback
Old configurations: new settings and hook items keep defaults; no stored format change beyond one more saved property.

## Deviations and owner decisions
See specification.md.
