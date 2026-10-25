# Task-scoped findings

## Storage and ownership

Version 2 uses `.factory/findings/registry.json` containing `{ "version": 2, "storage": "per-record" }`, individual active files in `.factory/findings/active/<ID>.json`, and individual closed files in `.factory/findings/archive/<ID>.json`. There is no aggregate active list or global allocation counter. Each origin task has one writer and owns its finding namespace; changes to different findings touch different files. Concurrent changes to the same finding require deliberate Git reconciliation.

At startup use `./.factory/scripts/findings list`, which emits active summaries only. Context export includes the marker and individual active records; it never includes closed payloads automatically. `show ID` explicitly reads one record. `check` internally validates active/archive integrity, duplicate cycles and retained task evidence without printing report payloads. Never recursively load the findings archive into normal context. Records contain concise observations, ownership and evidence pointers, not secrets or raw logs.

## Allocation

New IDs are `FND-<origin-task-id>-<local-sequence>`, for example `FND-20261022143012-a81f-fix-camera-001`. The sequence starts at 001, grows within that task, and is never reused after closure. The origin reference remains in the record even when another task fixes the defect. Origin tasks must use canonical ordered IDs; migrate legacy task names before creating new task-scoped findings. Existing delivered `FND-000001`-style IDs remain stable.

Prepare a JSON object with the fields in [finding.json](../templates/finding.json), removing its example `id`, then run from the matching unfinished origin task branch:

```sh
./.factory/scripts/findings create --task TASK-ID --record /outside/project/finding.json
```

Creation allocates and atomically publishes a complete active record under the cooperating local lifecycle lock; it validates the origin/task report and refuses replacement. `next-id --task TASK-ID` is a read-only suggestion and does not reserve an ID. Independent tasks have separate namespaces, so they do not edit or reconcile one shared counter. The task-ID collision limits still apply across independent clones.

## Record contract and lifecycle

Records retain exactly `id`, `title`, `category`, `severity`, `status`, `owner`, `blocking`, `tasks`, `summary`, `evidence`, `resolution`. Categories are functional/security/quality; severities critical/high/medium/low/informational. Active statuses are open/in_progress/blocked/deferred, with null resolution. Keep an accountable owner, next action and discovery/repair report references. Creating a follow-up task never closes a defect. Reports use the same canonical finding ID.

Each task reference contains `id`, report basename and `revision`. A null revision resolves to exactly one live/archive report in the current tree. Before pruning that report, pin every active AND closed reference to its full retained commit SHA. Immutable evidence stays at its original task path within that commit; task archival is a move and does not itself require pinning. Missing reports/commits and ambiguous locations fail validation. Planned tasks without reports belong in prose, not structured evidence references.

After a verified fix or valid disposition, prepare a complete copy of the record with status closed and resolution to `{ "kind": ..., "evidence": ..., "approved_by": ..., "duplicate_of": ... }`, then archive it with the lifecycle helper in one reviewed change:

```sh
./.factory/scripts/findings archive FINDING-ID --record /outside/project/closed-finding.json
```

The helper validates the proposed closed record, report references and duplicate chains, and moves it from active/ to archive/ under the lifecycle lock. A failed write restores the record. This command records a verified disposition; it does not perform verification or authenticate owner risk acceptance. Do not manually move finding records.

Xcode projects must not reference `.factory/` or its contents. Finding creation, archival and validation operate only on the registry and retained evidence; they never read or update Xcode projects. The former `findings sync-project` command and navigator helper are retired. Remove existing Factory navigator references in a reviewed project task while preserving the actual metadata and unrelated application/build objects.

Kinds are fixed, duplicate, not_a_bug or accepted_risk. Evidence is mandatory; accepted_risk additionally requires explicit human owner authorization with date/rationale/follow-up. Agents cannot accept risk. duplicate_of identifies another known finding only for duplicate closure, and cycles are forbidden. Preserve ID and content; never leave competing active/closed copies. Reopening moves the committed closed record back with the same ID, an active status and null resolution.

Before and after pruning task reports, run:

```sh
./.factory/scripts/findings check --prune-task TASK-ID
./.factory/scripts/maintenance check --prune-task TASK-ID
```

Pin both finding reports and durable completion-state evidence first. Keep the evidence commits in candidate ancestry; do not squash away the only copy. Neither command mines defects, performs tests, authenticates approvals or grants pruning/publication authority.

## Upgrade and new projects

Legacy version 1 active.json/counter/archive validation remains supported until the explicit [metadata upgrade](maintenance.md#upgrade-existing-projects). Migration splits the active list, preserves every legacy ID/closed record/pin, and retains the old counter/input in the upgrade task's evidence. Do not renumber delivered findings or import template findings into an adopting project. Template Git exports omit template finding record JSON while retaining the v2 marker and empty active/archive directories. A fresh destination starts with those empty directories; initialize that destination only, preserving template history and existing project findings. Task/state cleanup never clears findings.
