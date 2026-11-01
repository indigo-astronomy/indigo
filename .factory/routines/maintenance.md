# Maintenance from completed tasks

## Policy and durable evidence
Version2 maintenance.json stores policy/custom authorization, not mutable counters. New project policy separates incremental and full review:

```json
{
  "version": 2,
  "timezone": "Europe/Bratislava",
  "repository_review": {
    "review_every_task": true,
    "incremental_review_every_completed_tasks": 20,
    "full_review_every_completed_tasks": null,
    "full_review_every_calendar_days": 90
  }
}
```

`full_review_every_completed_tasks: null` disables only the full task-count trigger; quarterly calendar/significant-release review remains. Legacy policies without incremental settings keep their prior behavior. At startup/closure run maintenance status; --today supplies a deterministic owner-calendar date. This is workflow state, not a scheduler.

archive-task emits one immutable version1 completion per accepted completed task, under the lifecycle lock. Existing fields/records remain valid: version, task, completed_at, kind, audit and evidence_revision. Kinds distinguish work, audit (qualified full), incremental-audit and partial-audit. Audit-only kinds never count as work. Full/incremental records use explicit reviewed_on/revision/covered_tasks metadata; partial-audit has audit:null and exactly `Verdict: partial` in its retained repository report. An accepted partial inspection is a completed bounded outcome; blocked or genuinely unfinished work still cannot be archived.

Completion state/report proof and ancestry remain validated. Only explicit owner acceptance permits partial closure, with findings, actual coverage and unchecked-area follow-up retained; no security risk is thereby accepted. Before pruning, pin evidence_revision to retained completed state, validate maintenance check --prune-task before/after, and keep the completion. No aggregate counter is reset or written back.

## Distinct review checkpoints
A qualified full report alone advances full cadence. Default audit-input remains compatible:

```sh
./.factory/scripts/maintenance audit-input --revision FULL_REVIEWED_SHA --on YYYY-MM-DD
```

An incremental audit inspects changes since the incremental checkpoint/baseline, affected dependencies/contracts and open findings:

```sh
./.factory/scripts/maintenance audit-input --scope incremental --revision REVIEWED_SHA --on YYYY-MM-DD
```

These commands only prepare metadata; copy it to task-state completion after actual gates pass. Incremental input lists still-uncovered work completions visible at the reviewed snapshot; historical migration-baseline tasks are not relabelled reviewed. Do not add concurrent work absent from the reviewed source.

Status retains existing full-review fields and separately exposes incremental due/count/checkpoint fields. Full status considers qualified full audits only; incremental status combines actual full/incremental coverage with the explicit one-time counting baseline, if present. Partial closure advances neither actual checkpoint. Tie order remains reviewed date, completion time and task ID. Work unseen by an audit remains uncovered regardless of timestamps. Last full date/revision and historical uncovered work are preserved after the new policy begins.

## Explicit one-time cadence migration
An adopted project may record an owner-authorized immutable `incremental_review_baseline` object containing revision (retained full Git SHA), authority (nonempty owner authorization) and report_task (the partial report task). Resolve visible completion IDs from that retained snapshot and validate ancestry plus the referenced partial report. This is an explicit one-time policy migration, not a mutable checkpoint or review claim. Do not refresh it automatically. Keep its revision/task count separate from actual covered_tasks, retain unchecked-area follow-up, and never reset the full checkpoint.

INDIGO's approved start is2b040e9b6daa120b78459e3f91242b0848615716. The decision and rationale belong in ADR-004; execution/proofs belong in the policy task. Existing historical deferrals remain evidence, but the old20-task/seven-day full-audit requirement is superseded by the owner's new policy. Legacy timezone/configuration behavior remains supported.

## Upgrade existing projects

Coordinate the schema cutover: finish or explicitly hand off old-schema writers and synchronize completed branches before integration; never drop their records. Run legacy task-ID/state migration first. If that renamed tasks on the upgrade branch, retain a reviewed local checkpoint commit containing their completed task-local states before importing completion proof. This is a justified local migration checkpoint, not permission to push an unfinished task. Complete the entire upgrade before publication. The task renamer refuses incompatible already-migrated legacy completion namespaces instead of silently corrupting IDs.

Install maintenance.py, metadata_upgrade.py and their shared dependencies with the reviewed source-to-target map. All finding/completion/configuration data remains project-owned and protected from template replacement. Prepare a reviewed import manifest outside the project:

```json
{
  "version": 1,
  "template_export_baseline": false,
  "records": [
    {
      "version": 1,
      "task": "20261022143012-a81f-example",
      "completed_at": "2026-10-22T14:45:00+00:00",
      "kind": "work",
      "audit": null,
      "evidence_revision": "FULL_RETAINED_TASK_STATE_COMMIT"
    }
  ]
}
```

Use actual completion evidence, not task creation timestamps or invented placeholder history. Include every retained completed task, and historical pinned tasks needed to reproduce the existing checkpoint/count. For old full audits, import explicit actual reviewed date/revision/covered task IDs from their retained reports. The helper verifies completion states in pinned ancestry and requires the imported derived count/checkpoint to match legacy maintenance values. Missing or conflicting history must be reconstructed/reviewed explicitly; never reset a count because records are inconvenient. Custom policy/authorization fields remain in configuration. Legacy raw maintenance and findings inputs are retained inside the upgrade task.

```sh
python3 .factory/scripts/metadata_upgrade.py --plan UPGRADE-TASK-ID \
  --completions /outside/project/completions.json > /outside/project/metadata-plan.json
python3 .factory/scripts/metadata_upgrade.py --apply /outside/project/metadata-plan.json
```

Optional `--project ROOT` supports invoking a trusted template helper before installation. Plan creation writes nothing; application rechecks HEAD, branch, registry inputs and proposed destinations under the local lifecycle lock. Edited/stale plans fail; ordinary write errors restore files/directories. A crash requires inspected Git/plan recovery. Existing closed findings and provenance are untouched. After migration run findings/check, maintenance/check/status, context/lifecycle/review/update regressions, decision-index and committed-range process checks for the upgrade task, plus application checks.

`template_export_baseline: true` is reserved for the template repository's documented zero/null export baseline: import its actual retained audit/completion evidence rather than pretending export resets were audits. It is refused for adopted projects. Template maintenance retains real completion history in Git; template-only export attributes omit completion JSON records, template finding JSON and task reports while retaining empty directories. Adopted projects retain their own records and do not install those export exclusions.
