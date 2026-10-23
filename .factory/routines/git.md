# Git workflow for task branches

## Policy and ownership

Use one temporary `task/<task-id>` branch per task, including specification, implementation and evidence. Role transitions reuse that branch. The coordinator owns synchronization, integration, publication and cleanup. Discover the main branch, remote, protections and owner authorization in project.md; assignment authorizes local task work, not publication or merging into a shared target.

**Default: one outcome-named commit per completed task.** Include implementation, tests, documentation, review reports, process evidence and the completed task's archive move together. Planning, role changes, recording checks, pinning evidence and archival do not require separate commits. Name the commit for the user-visible or maintained behavior, not internal process steps.

A recoverable checkpoint for interruption/handoff, an opt-in commit-bound cross-provider review, or independently useful changes may justify more commits. Record the reason in handoff.md; no extra owner gate is needed for routine choices. Never rewrite published history. Do not automatically amend/rebase/squash shared or published task commits. Before publication, an explicitly authorized consolidation must preserve all required evidence and reverify the resulting candidate; consolidation is unnecessary for the default workflow below.

Do not force-push, reset --hard, git clean, branch -D, bypass protection, or discard unrelated work. This is a manual procedure, not an automatic hook.

## 1. Start safely

Inspect status, branches, remotes, worktrees and factory state. Preserve existing changes. Fetch the configured remote when authorized, confirm the local integration branch can fast-forward, and create the task branch from that updated base before task writes. Preserve divergence for explicit resolution. Resume an existing task on its branch; never replace it with switch -C or branch -f. Record base/target/remote in handoff.md, then run ./.factory/scripts/new-task TASK-ID. Allocate the ordered ID first with `new-task SLUG --print-id`; see [concurrent-tasks.md](concurrent-tasks.md).

```sh
git status --short --branch
git fetch origin
git switch main
git merge --ff-only origin/main
TASK_ID=$(./.factory/scripts/new-task example --print-id)
git switch -c "task/$TASK_ID"
./.factory/scripts/new-task "$TASK_ID"
```

These branch/remote names are examples. Read-only adoption discovery can precede Git bootstrap. One writer owns the task checkout; concurrent tasks need separate branches/worktrees and file ownership. Do not update main in another writer's worktree or overwrite another task's state.

## 2. Work and synchronize

Keep in-progress changes in the task working tree. Inspect staged and unstaged changes; stage assigned paths only. Complete specifications, checks, documentation and reviews without making administrative commits.

Fetch the target again before final verification. If it advanced, synchronize on the task branch. With uncommitted work, merge only when Git can preserve it safely; otherwise preserve a documented checkpoint commit or use an authorized separate worktree. Do not automatically stash or discard changes. Resolve conflicts deliberately, then rerun affected tests and reviews. A changed target never permits stale evidence. For local-only projects, use the configured local main target.

If synchronization would create extra task commits, record the concurrency/checkpoint reason rather than rewriting published history to meet a numerical quota. Reconcile shared state/counters instead of blindly choosing one side.

## 3. Review content and prepare retention

Run applicable checks on the synchronized working tree. Complete documentation, quality review and security sign-off, with limitations recorded. New tasks use version 2 content evidence under [process-checks.md](process-checks.md); it binds reviewed paths to mode/type/object identity without needing a source commit.

After acceptance/review gates, mark task execution complete while recording integration/publication as pending. Reconcile findings and maintenance once, then run [task archival](task-archive.md) on the same branch before the final commit. Template maintenance follows [template retention](template-maintenance.md). Review incoming-link changes from archival; source/document changes require affected review/check refresh. Archive placement is retention, not delivery approval. Unfinished tasks remain live.

Stage assigned source files, capture their reviewed identities with `--record-content`, place the JSON object in process.json's `reviewed_content`, and stage final evidence. Inspect the actual index/diff and worktree. The recorder reads staged content only: reviewers must inspect that content, and unstaged source changes must be resolved before committing. Capture is not a review or a test and executes no recorded command. Empty/unconfigured required check groups remain unconfigured.

## 4. Commit once and verify

Create the single outcome-named commit containing the final source and archived evidence. Run required committed-candidate checks, decision-index/findings validation and the committed-range process check with a clean worktree. Record the final SHA and post-commit results in the external delivery/CI log; do not create another commit solely to insert that SHA or restate passing checks in its own reports. Reports identify reviewed content, executed pre-commit checks and the required post-commit delivery gates honestly.

A failed committed-candidate gate blocks integration. Fix and refresh affected evidence; preserve published history. Any additional repair commit needs a concrete reason, never a claim that an earlier candidate passed. Unperformed checks are not passes.

## 5. Integrate exactly the verified candidate

Fetch immediately before integration. Compare the target against the revision included during verification. If it advanced, return to synchronization and verification. Direct integration requires owner authorization, a clean worktree and unchanged tested task head:

```sh
git switch main
git merge --ff-only origin/main
git merge --ff-only task/20261022143012-a81f-example
git push origin main
```

Confirm HEAD equals the tested task commit and required publication succeeds. A rejected push preserves local work; fetch, synchronize on the task branch and reverify without force. Do not fall back to an unchecked normal merge, squash or rebase. Distinguish ready, locally integrated and published results.

For protected branches, use the required PR/check/merge-queue policy. Squash/rebase changes the candidate identity: validate the actual resulting candidate, including retained v2 content evidence, rather than claiming the old task SHA was integrated unchanged. Commit-bound v1 cross-review evidence and pinned findings may require retaining their source ancestors; do not discard them. Missing evidence/authorization leaves a ready branch, not completed delivery.

## 6. Cleanup after delivery

After verified integration and required publication, delete completed local task branches with `git branch -d`, then inspect `git branch --list`. Keep only the configured integration branch and explicitly assigned unfinished branches. Preserve unmerged work; do not use -D to hide squash/rebase ancestry differences. Record any concrete cleanup blocker. Remove clean owned task worktrees through the supported host lifecycle before deleting their branches. Remote branch deletion requires applicable authorization and confirmed merge. Cleanup remains pending while integration/publication is pending.

The committed handoff may state that task execution/reviews are complete and integration is pending. Confirm actual integration, publication and branch cleanup externally after they happen; no bookkeeping-only follow-up commit is required.
