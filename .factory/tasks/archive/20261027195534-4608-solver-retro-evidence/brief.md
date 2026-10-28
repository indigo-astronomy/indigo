# Task brief

- Task ID: 20261027195534-4608-solver-retro-evidence
- Owner: Peter Polakovic
- Created: 2026-10-27

## Problem and desired outcome
Commit `a1e7f53d08bcec9b25309f1a2944d39451572253` on `master` ("agent_solver: filter noise before star detection
and accept FITS, JPEG and camera raw input", Solver Agent version 3.0.0.7, authored 2026-10-27 19:55 +0100) was made
and pushed outside the AI Factory workflow: no task ID, no `task/<id>` branch, no task record, no specification, plan
or test plan, no verification, quality or security report and no process evidence. The owner stated in conversation
on 2026-10-27 that the Factory rules must always be followed and agreed that a retroactive evidence task is the
right repair. History must not be rewritten (root `AGENTS.md`, Repository Hygiene). The desired outcome is an
archived task record that describes what that commit delivered, re-runs the checks that can be re-run, reviews the
published diff after the fact with its independence limitation stated, and records process evidence that the
committed-range checker accepts or whose remaining limitation is documented.

## Users and use cases
Maintainers and future agents who derive maintenance cadence, findings references and change history from the
Factory records; the owner, who needs the published change to be reviewed and traceable like every other task.

## Scope and non-goals
In scope: this task's record under `.factory/tasks/`, the owner's audit deferral entry in `.factory/maintenance.json`,
the completion record written by archival. Non-goals: any change to application source, tests or documentation
(the reviewed commit is published and stays as it is), a recorded driver test run (the README `## Testing` record
already holds the run of 3.0.0.7 and a new run would record 3.0.0.8), the later Solver Agent task
`20261027192244-16bd-solver-hints-input-diagnostics` (recorded separately), integration into `master` or a push.

## Constraints and references
Root `AGENTS.md`, `.factory/bootstrap.md`, `.factory/workflow.md`, routines `git.md`, `concurrent-tasks.md`,
`quality-review.md`, `process-checks.md`, `task-archive.md`, `findings.md`; `indigo_drivers/AGENTS.md` and
`indigo_test/AGENTS.md` for the reviewed paths. Evidence of the reviewed change: `git show a1e7f53d0 --stat`,
`indigo_drivers/agent_solver/REFACTOR.md` sections "Noise reduction before star detection" and "Input formats",
the README `## Testing` record "2026-10-27 19:48 3.0.0.7 mac arm64 simulator 141/141 OK" as committed in
`a1e7f53d0` (replaced since by the 3.0.0.8 record). The full repository audit is due; the owner's deferral for this
housekeeping task is asked in chat and recorded in `.factory/maintenance.json` like the earlier entries.

## Initial questions
- Owner deferral of the due full repository audit for this housekeeping task (asked in chat).
- Whether version 1 process evidence with `reviewed_revision` = `a1e7f53d0` passes the committed-range checker for
  a task whose own range adds a completion record: resolved in plan.md and handoff.md.
