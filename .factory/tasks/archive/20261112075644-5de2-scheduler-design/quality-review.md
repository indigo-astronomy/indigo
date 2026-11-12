# Task quality review

Verdict: pass
Reviewer and independence: independent read-only reviewer subagent (fresh context, Claude Code general-purpose agent) for the first pass; the coordinator (same model that wrote the document) applied fixes and performed the fix reinspection. The reinspection is not independent.
Reviewed revision and working-tree state: branch task/20261112075644-5de2-scheduler-design from master e0280e650, uncommitted `indigo_docs/SCHEDULER.md` (new) and `indigo.xcodeproj/project.pbxproj` (2 added lines).

## Scope and method
The new design document and the Xcode project change. Every claim about existing code was checked against `indigo_drivers/agent_scripting/library/Sequencer.js`, `Sequencer.md`, `indigo_agent_scripting.c`, `indigo_names.h`, `indigo_align.h`, `indigocat/indigocat_solar_system.h`, `indigo_filter.h` and `indigo_agent_mount.c`. The external survey facts were not verifiable from the repository; they come from the cited product documentation gathered during planning.

## Quality findings
First pass: 1 blocking, 17 should-fix and 6 nits (reviewer numbering 1–24). Summary:
- Blocking: the examples called `sequence.set_batch()`, which is not part of the `Sequence` API; `capture_batch(count, exposure)` is correct (Sequencer.js:364).
- Should-fix: the M31 example did not move to its target and duplicated coordinates; start and end detection did not handle a `STOP_TIMEOUT` expiry or RUN_SCRIPT failure; stored urgent tasks contradicted "no decisions while running"; preemption did not require the urgent task to be eligible; urgent submission while stopped, unstored urgent task visibility and cancellation, the guider and other agents to abort and wait for, state leaking between tasks (meridian flip), early-break success counting, persistence of counters, edit, rename, RESET, ENABLE and hook semantics, scoring constants, header grammar edge cases, and units and defaults were unspecified.
- Nits: J2000 conversion and missing moon phase helper, RETRY/REPEAT routing mismatch between diagram and text, embedded library build details, two-step command race, header blank line, body trust level.
No finding concerns code; none was entered in the central findings registry because all were resolved in this task's draft before delivery.

## Fix reinspection
All 24 items were addressed in the revised document: corrected examples using `capture_batch()` and a `scheduler_task` object built from the header; new subsections for lexical rules, Lorentzian formula, editing, enabling, urgent tasks, persistence (`AGENT_SCHEDULER_RESTORE_STATUS`), scheduler control, hooks, state between tasks and aborting a sequence; scoring constants and weights in SETTINGS; `NEXT_START` moved to a text item with a stated format; settings defaults; aligned state routing. Example API calls (`enable_meridian_flip`, `disable_meridian_flip`, `precise_goto`, `start_guiding`, `stop_guiding`, `select_filter`, `capture_batch`) were rechecked against the `Sequence.prototype` list.

## Quality verdict
Pass for a design document. Remaining limitation: property names, limits and defaults are provisional and stated as such; the external survey is not repository-verifiable.
