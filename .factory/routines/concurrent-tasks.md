# Concurrent tasks and ordered identities

## Identity and execution state

Version 2 uses `YYYYMMDDHHMMSS-hhhh-slug`, for example `20261022143012-a81f-fix-camera`. The fourteen-digit prefix is the creation time in UTC at second resolution; the four lowercase hexadecimal digits contain 16 random bits. Sort directories lexically to group tasks by represented time. Tasks within the same second have no creation-order guarantee. Clock skew between hosts affects ordering; this is not a distributed logical clock.

The owner selected the compact suffix. It provides probabilistic identity across independent clones, not guaranteed global uniqueness. Local allocation retries collisions against live/archive directories, available Git history and task branches (up to 128 attempts, then fail explicitly). An ID printed before branch creation is a proposal, not a reservation. A matching Git branch reserves it across worktrees sharing the repository. Never reuse an ID. If separate clones collide, rename the incoming unintegrated task and repair its mutable references before integration; do not combine their reports or overwrite either task.

`.factory/state.json` is now a version 2 repository marker with `task_state: "per-task"`. Each live task owns `.factory/tasks/ID/state.json`, retaining the standard `active_task` (its own ID), phase, status, assigned_role, next_action and blocker fields. Archived task state stays with its reports. Global state has no active pointer; several tasks may be active. Custom legacy global fields survive migration, and the active execution state retains them too. Do not overwrite global state during role transitions.

## Allocate, isolate and assign

Choose the integration branch from project.md and synchronize it under the existing Git permissions. Allocate an ID before creating a branch/worktree:

```sh
TASK_ID=$(./.factory/scripts/new-task fix-camera --print-id)
git worktree add -b "task/$TASK_ID" /path/to/task-worktree MAIN_TARGET
/path/to/task-worktree/.factory/scripts/new-task "$TASK_ID"
```

Replace MAIN_TARGET and the worktree path with the assigned values. If branch creation reports an existing branch, allocate a fresh ID; never force/reset a branch. A single-task checkout can use `git switch -c "task/$TASK_ID" MAIN_TARGET` instead. `new-task SLUG` can allocate and create directly, but the coordinator must still assign the matching task branch before implementation.

One writer owns each task/worktree. Discover the current task from the matching `task/ID` branch or an explicit assignment, then read its own state and handoff. Do not choose the latest directory as the active task. Separate worktrees isolate source files; a lock does not make concurrent writes to one checkout safe.

Creation, archival and task migration acquire an atomic directory lock at Git's common directory (`factory-task.lock`), shared by linked worktrees. Outside Git the lock is `.factory/.task-lock`. Contention fails immediately without taking over another writer. Owner metadata records PID and host. If a process is killed, inspect the owning process and checkout before manually removing a stale lock; no timeout steals it. The lock coordinates cooperating local lifecycle helpers, not editors, findings updates, Git integration or other clones. A crash can require manual recovery; ordinary migration failures attempt rollback and report failure explicitly.

Findings use task-scoped IDs and individual files; maintenance derives from independent completion records under [maintenance.md](maintenance.md). Keep one coordinator for shared configuration and integration. Before integration synchronize, preserve both independent records, reconcile shared configuration and inspect incoming task IDs for collisions, then reverify the final candidate. No distributed lock, scheduler, automatic merging or additional authority is introduced.

## Upgrade existing task IDs and state

On the project upgrade branch, install the reviewed `task_store.py` and `task_upgrade.py` alongside the other helpers. Version 1 helpers remain supported until this explicit migration; legacy new-task still requires an idle global state and a supplied ID. Include the new modules and this routine in the reviewed template map without replacing project-owned data or losing the old three-way baseline.

Freeze other task writers and Git operations in this repository for the migration. Require an unfinished upgrade task on its matching branch; hand off any other legacy live tasks explicitly first. Archive and findings trees are read only as explicitly selected migration input, not routine startup context.

```sh
python3 .factory/scripts/task_upgrade.py --plan OLD-UPGRADE-ID > /tmp/task-upgrade-plan.json
# Review mapping, time_source and every proposed task identity.
python3 .factory/scripts/task_upgrade.py --apply /tmp/task-upgrade-plan.json
```

The helper also accepts `--project /absolute/project` when invoked from a trusted template checkout before installation. Keep the plan outside the project: its inventory fingerprints supported Git-visible text files plus selected task/findings text evidence (each limited to 1 MiB; plan input limited to 16 MiB), so saving it inside the task would invalidate the plan. Plan creation writes nothing. Application rechecks the branch, HEAD, input contents/modes, task inventory and collisions under the local lifecycle lock. Drift requires a fresh reviewed plan; do not edit the plan to bypass it.

Every legacy archive directory and the active upgrade directory receives an ordered ID. Time comes from the first retained Git addition's author timestamp, or the brief's modification time for uncommitted records. The plan labels this fallback for review. Historical suffixes are deterministic 16-bit hashes with collision resolution so rerunning an unchanged plan is stable. Existing canonical IDs remain unchanged. The active upgrade branch is renamed to `task/NEW-ID` together with its task. Continue every subsequent updater/context/review/process command with the printed NEW-ID; do not create a replacement task. A repeated migration on a fully migrated repository is a no-op.

The migration updates mutable text references, manifests' task identities/scopes, Markdown links, maintenance checkpoint IDs and findings references with null revisions, while retaining custom values and findings counters. It preserves template-lock provenance, process `reviewed_content`, review evidence arrays, `reviews/` packets, external URLs and references containing full commit pins. Historical reports retain their original review claims; renamed manifests do not mean they reviewed today's source. Commit-pinned findings keep their old task IDs because the original reports remain at those paths in the pinned commit. Git history is never rewritten.

Binary and unsupported-format reports move unchanged and are not loaded into the text rewrite inventory. Review project-specific references in other formats manually. Review the complete migration diff, including pinned-reference exceptions and project-specific references outside supported text formats. Regenerate current review packets/process evidence when context or source changes; preserve historical evidence. Run findings validation, lifecycle/context/review/update regressions, decision-index checks, application checks and committed-range process checks for the upgrade task. Preserve old evidence and provenance in Git for rollback; restore through a reviewed reverse migration rather than discarding unrelated edits.
