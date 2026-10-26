# Completed task archival

## Ownership and lifecycle

Ordinary projects retain completed task evidence in `.factory/tasks/archive/<task-id>/`. Live `.factory/tasks/<task-id>/` directories hold unfinished or blocked work. Each version 2 task retains its own `state.json`; the global repository marker never selects an active task. Legacy version 1 uses its global pointer until explicit state migration. Do not load task or findings archives during routine startup, recursive searches or scoped-instruction discovery. Use explicit archive exclusions with permitted search tools; root instructions prohibit `rg` and `ripgrep`. Read a specific archived record only for a concrete historical question or referenced checkpoint; helper integrity checks may inspect archives internally without dumping their contents into context.

The coordinator first verifies all acceptance/review gates, reconciles findings, derives maintenance status from completion records, and completes handoff evidence with Git delivery still pending until confirmed externally. Archive before the single final task commit. Archival is retention housekeeping, never approval, risk acceptance or proof of publication. Never archive unfinished work. The helper checks that `active_task` selects the requested task, that both `phase` and `status` equal `"complete"`, and that `blocker` equals JSON `null`; human/coordinator gates remain responsible for the truth of that declaration.

```sh
./.factory/scripts/findings check
./.factory/scripts/archive-task 20261022143012-a81f-example --dry-run
./.factory/scripts/archive-task 20261022143012-a81f-example
./.factory/scripts/findings check
```

Run from the project's checkout (the helper resolves its own repository root). Archive in the same coordinator-owned task branch before its final result commit under the existing Git policy; if delivery was already completed, create an authorized housekeeping branch rather than writing main. Review the move/link diff, refresh affected reviewed content, and include it with the implementation/evidence in the single final commit; verify that committed candidate. Do not delete task branches until their required delivery is confirmed. No commit, merge, push, gate evaluation or maintenance counter update occurs in this helper.

The archive destination must be unused, inputs must not be symlinks, and standard workflow fields must be present in a JSON object with unique keys and finite values; `NaN` and infinities are rejected. The helper preserves custom task files and project-owned state fields, updates simple relative Markdown links, moves the directory, and preserves completed task-local state in version 2. For legacy version 1 only, it clears the global assignment to idle/coordinator. It preflights inputs and restores edited files/state and the original task location on ordinary write failures. If rollback fails or the process is killed, preserve both locations, inspect Git/worktree evidence and repair explicitly before retrying. It is not a crash-atomic multi-file transaction; one writer per checkout is required.

## Links, IDs and context

Apply AGENTS.md's persistent-context rule when reviewing the move: project.md and shared procedures keep only current conclusions and evidence references. Verify incoming references resolve to the archived task, including plain-text references the helper cannot rewrite. A reference does not authorize loading that task's history during routine startup.

Findings references keep their existing `{id, report, revision}` schema. A null revision resolves to one live or archived task; a full commit SHA resolves to exactly one report blob at either location in that historical commit. Pins from before a move remain valid. Duplicate live/archive task locations are an error. Moving reports does not require pinning; deleting them later still requires pinning active AND closed finding references and passing `findings check --prune-task ID` before and after deletion. Never reset the findings register, counter, archive or maintenance history during archival.

The archiver uses `git ls-files` to inspect tracked and nonignored untracked Markdown files, including historical task documents internally, and all Markdown files in the moved task. It updates simple inline links/images and reference-definition destinations relative to their new locations, including incoming links, outgoing links, query strings, fragments and percent-encoded spaces. URLs with schemes, absolute paths and anchor-only links stay unchanged. It does not parse arbitrary HTML, wiki links, complex nested Markdown syntax, ignored backlinks outside the task or links in other formats. Review those manually; Git-visible symlink Markdown inputs or oversized/non-UTF-8 text stop the operation before writes. Text input limit: 1 MiB per file. No external URL fetching occurs.

`new-task` rejects the reserved ID `archive`, existing live/archive IDs (including dangling symlinks), and IDs appearing in available local Git refs/history when Git is installed. This is local evidence, not a global ID allocator: fetch authorized refs before choosing IDs and coordinate across branches/shallow clones. A non-Git project can only check current live/archive locations. Do not delete archive records merely to reuse an ID.

The context exporter does not traverse the task archive or load archive-scoped AGENTS files through `--path`, including symlink aliases. Historical inspection requires explicit selection:

```sh
./.factory/scripts/context --task 20261022143012-a81f-example --archived --role coordinator
```

This includes only that archived task's standard reports alongside normal shared policy/current state. It labels the task historical and does not resume it. Without `--archived`, an archived-only task fails with no partial output. Resuming delivery of a completed execution uses its existing branch and explicitly selected archived handoff, with final outcomes recorded externally; do not create a duplicate task or claim publication from archive placement. Reopening implementation work requires a deliberate coordinator assignment and preservation of the prior evidence; do not simply point active state into the archive.

## Existing projects and template maintenance

Preserve existing archived and live records during upgrades; `.factory/tasks/` is protected project-owned data. There is no bulk migration or automatic scheduler. For a previously completed live task, inspect its final handoff/reviews and delivery evidence first. Preserve any current assignment in its handoff and finish or explicitly hand it off; never overwrite another writer's state. As coordinator, select the verified completed ID with complete phase/status and no blocker, run the preview/move/check sequence, review links and commit under the project's policy. Repeat deliberately for other completed tasks, without recounting maintenance completions. Unfinished and blocked tasks stay live. Never mark an unfinished task complete just to enable the command.

The AI Factory template itself follows [template-maintenance.md](template-maintenance.md): retain completed records in the versioned archive, excluded by template-only export attributes from new-project Git exports. It does not ship its own completed task archive into adopting projects. A new project starts without inherited task evidence, while existing projects retain their own archive and maintenance data.

## Durable completion evidence

With maintenance version 2, successful archival atomically adds `.factory/completions/ID.json` under the lifecycle lock and rolls it back with the move on ordinary failure. Publish the completion with the fully reviewed task, never an unfinished task. Existing records cannot be overwritten. See [maintenance.md](maintenance.md) for qualified audit metadata, derived counts and completion-state pinning before pruning; retain completion records after deleting reports.
