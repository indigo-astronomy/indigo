# Maintenance from completed tasks

## Policy and durable evidence

Version 2 `.factory/maintenance.json` contains policy and custom project settings, never mutable aggregate counters or checkpoint pointers:

```json
{
  "version": 2,
  "timezone": "UTC",
  "repository_review": {
    "review_every_task": true,
    "full_review_every_completed_tasks": 5,
    "full_review_every_calendar_days": 7
  }
}
```

Choose the owner's calendar timezone during project setup. Run `./.factory/scripts/maintenance status` at startup and closure; `--today YYYY-MM-DD` makes historical/boundary checks explicit. It derives counts/checkpoints from `.factory/completions/<task-id>.json`, one versioned record per completed task. No aggregate is written back. Different task branches add different files and merge both completions. Only completed tasks are published to the shared repository; the committed process gate validates completion proof and rejects task states without a completion record. Record creation is not proof that the claimed checks/reviews occurred.

After acceptance/reviews, mark task state complete and run `archive-task`. Under the lifecycle lock it moves the task and emits its completion record, rolling both back on ordinary failure. The record contains `version: 1`, `task`, UTC `completed_at`, `kind` (work/audit), `audit` (null or qualified full-audit metadata) and `evidence_revision` (null or a full retained source-state commit). Repeated completion cannot overwrite an existing record. The automatic timestamp is completion time, not task creation time. A partial/blocked task never contributes a completion.

Completion evidence is durable independently of reports. Before pruning a task's state/reports, pin its completion `evidence_revision` to a retained commit containing exactly one completed task-local state; validate with `maintenance check --prune-task ID` before and after pruning. Keep completion records after report pruning. Existing full audit and finding risk-acceptance gates remain unchanged.

## Audit coverage and due status

A report filename alone does not qualify as a full audit. After completing the required full-repository audit, produce explicit task-state `completion` metadata using the actual reviewed commit/date:

```sh
./.factory/scripts/maintenance audit-input --revision FULL_REVIEWED_SHA --on YYYY-MM-DD
```

The read-only result has `kind: "audit"` and `audit: { "reviewed_on": ..., "revision": ..., "covered_tasks": [...] }`. Copy it to the audit task state's `completion` field only after coverage/finding gates pass. For a work task also containing a full audit (such as adoption), use kind work with the same explicit audit metadata. The source commit must be retained; the command lists completion IDs visible at that reviewed commit. Do not expand coverage to tasks merged later without actually reviewing them.

Due status uses the latest audit by reviewed calendar date, completion timestamp and task ID (deterministic tie order). It counts work completions outside that audit's explicit `covered_tasks`; audit-only tasks do not count. An audit within a work task does not cover its own subsequent completion. A task finished concurrently on another branch remains uncovered even if its clock timestamp is earlier. This prevents a new audit checkpoint from silently forgetting unseen concurrent work. Five uncovered work tasks, seven calendar days, or no completed audit means an audit is due. Same-second completions are counted independently. This is derived workflow state, not an automatic scheduler or authenticated approval system.

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
