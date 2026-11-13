# Verification report

Verdict: pass
Owner review: design reviewed and approved by the owner 2026-11-13
Reviewer: coordinator (Claude Code, claude-opus-5-5) and an independent read-only reviewer subagent (fresh context, same model)
Date: 2026-11-13
Revision and working-tree state: branch `scheduler` at 8983cf9b7 with this task's uncommitted changes
Review independence: the subagent received the diff and the code, not the writer's conclusions.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | SCHEDULER.md "Night hooks (planned)": night limit, offsets, merged nights, no-night and polar cases, flags, order, idle periods, shared tasks, unnamed hooks, failures, stop, plan, history, persistence |
| AC-002 | pass | "The night limit times only the night hooks"; Rules: night hooks never interrupt a task |
| AC-003 | pass | survey row "Night start and end"; adopted list; phase 5, safety 6, extensions 7; phase-5 test bullet |
| AC-004 | pass | guide "Several nights" matches `control()` (STARTUP once per START), stop handling, idle period (`idle_since`, `idle_hook_done`), `unpark_before_coordinates()` via `slew_process()`/`mount_dome_control()` with dome slaving, `enable_cooler(temperature)` and `dome_open()` in Sequencer.js |
| AC-001a | pass | "Nights without work" rule; night flag, table, plan bullet and tests updated (owner decision 2026-11-13) |
| AC-005 | pass | anchor script: no broken link; `git diff --check` clean |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| Python anchor and cross-document link check | repository root | 0 | `broken: []` |
| `git diff --check` | repository root | 0 | clean |
| `./.factory/scripts/findings check` | repository root | 0 | `Findings valid: 30 active, 296 archived; task evidence checks passed.` |

## Baseline comparison and regressions
Documentation only.

## Found defects
| Defect | Impact | Cause | Fix | Regression test | Found by |
| --- | --- | --- | --- | --- | --- |
| Empty `NIGHT_START` cleared the standby | `RESUME` skipped after `IDLE` | unnamed hooks changed standby | unnamed hooks change only the night flag | design text | reviewer |
| Order rule listed `RESUME` after `NIGHT_START`, omitted `IDLE` | ambiguous order | | order rewritten | design text | reviewer |
| `IDLE` could not run after `NIGHT_START` | equipment left ready all evening | idle period not restarted | night hooks begin a new idle period | design text | reviewer |
| Failed `NIGHT_START`/`RESUME` stopped without `SHUTDOWN` | roof could stay open | modelled on failed `STARTUP` | `SHUTDOWN` runs, then Alert stop | design text | reviewer |
| "once per night" vs STOP/START | contradiction | | "once per night and `START`" | design text | reviewer |
| Missing cases: missing task, polar night, merged nights, later no-night | undefined behaviour | | rules added | design text | reviewer |
| Plan horizon (sunrise) vs night end | hooks outside the plan | | plan ends at the later of both; recompute on NIGHT/HOOKS | design text | reviewer |
| Persistence of `AGENT_SCHEDULER_NIGHT` unspecified | | | save trigger and order specified | design text | reviewer |
| Guide: dome unpark unconditional; `enable_cooler()` without argument; dawn idle without `@twilight` | misleading | | conditions and argument added | code reading | reviewer |

## Manual checks and limitations
The design is not implemented or executed; behaviour claims about the future agent are design statements only.

## Verification verdict rationale
All acceptance criteria pass after the review fixes; the design itself awaits owner review.

## Final test summary
- Simulated tests: none (documentation).
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
No findings; design defects were fixed in the draft.
