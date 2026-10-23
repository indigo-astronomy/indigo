# Helper scripts

Factory-owned helpers live together in `.factory/scripts/`. Keep project-owned scripts separate. Commands below are relative to the project root; invoking an absolute wrapper path from another working directory still selects that project. For old installations, follow [the relocation migration](../routines/template-updates.md#move-existing-factory-helpers-into-factoryscripts).

Requires Python 3.9+ on PATH. Run scripts directly on macOS/Linux or invoke the .py helpers with Python on other platforms.

- ./.factory/scripts/new-task SLUG: allocate a UTC second-resolution `YYYYMMDDHHMMSS-hhhh-slug` ID (four random hex digits) and create task-local state in version 2. `--print-id` allocates without writing for matching branch/worktree setup; passing an allocated ID creates that exact task. Legacy version 1 retains explicit-ID/global-state behavior. IDs in live tasks, archived tasks or available local Git history are never reused; `archive` is reserved. Existing tasks are never overwritten.
- ./.factory/scripts/check: run the configured check group.
- ./.factory/scripts/test: run the configured test group.
- ./.factory/scripts/security: run the configured security group.
- ./.factory/scripts/findings: validate findings/task-pruning evidence or inspect selected records.
- python3 .factory/scripts/run_checks.py GROUP: portable direct equivalent.
- python3 .factory/scripts/new_task.py TASK-ID: portable task creation. State input is limited to 1 MiB and must be a JSON object with unique keys and finite values. State/output path components and every template entry must be regular files or directories, without symlinks; invalid inputs are rejected before task creation.

## Decision navigation and delivery evidence

`python3 .factory/scripts/decision_index.py` generates `docs/decisions/README.md` from canonical ADR metadata; `--check`
returns 1 if stale without writing. `python3 .factory/scripts/process_check.py --base BASE_COMMIT --head HEAD` validates
committed task evidence, reviewed source identity, findings and diff risk. The POSIX wrapper is `./.factory/scripts/process-check`.
`--record-content --base BASE --task TASK-ID` prints staged source identities for a version 2 manifest without writes; `--risk-only` describes risk without checking completion; `--show-policy` prints defaults for the project-owned
`.factory/process-policy.json`. Both helpers require Python 3.9+; the process check also requires complete local
Git history and the offline flags used by template updates. No network, provider calls or recorded commands execute.

New tasks receive an unfilled `process.json`, never an automatic pass. The [shared routine](../routines/process-checks.md)
defines the schema, final-report requirements, baseline configuration, optional CI invocation and limitations. Gate success is
structural evidence only; ordinary work gains no extra owner approval step. Keep generated indexes current and retain
archived reports with the source changes in the single final task commit. Project-specific tests remain separately configured.

## Completed task archival

`./.factory/scripts/archive-task TASK-ID --dry-run` validates and previews the move; omit `--dry-run` to move the selected task from `.factory/tasks/TASK-ID/` to `.factory/tasks/archive/TASK-ID/`. Python equivalent: `python3 .factory/scripts/archive_task.py`. The selected phase/status must both be complete with no blocker. The helper updates simple relative inline/reference Markdown links in Git-visible `.md` files (plus all task Markdown files), moves custom reports, preserves maintenance/findings and version 2 task-local completion state; legacy version 1 clears the workflow pointer. It requires local Git for file inventory, rejects symlinks in inputs, and returns 2 on failure. It performs no commit, approval or delivery operation. Archive after acceptance/reviews but before the final task commit, with delivery pending. Review the diff and refresh affected content identities; see [archival and migration details](../routines/task-archive.md).

## Portable context export

./.factory/scripts/context --task TASK-ID --role ROLE --path FILE-OR-DIRECTORY writes a local Markdown snapshot to stdout. The role defaults to coordinator; --path can be repeated to add ancestor scoped AGENTS.md files. It includes only canonical factory instructions and standard task reports, not source code, arbitrary file contents, native rule globs, or external ancestor instructions. It performs no network/Git/test execution and never edits state. See ../portability.md for the sharing/handoff protocol.

Task archives are omitted unless `--archived --task TASK-ID` explicitly selects one historical task. `--path` never loads archive-scoped instructions, including through aliases.

Errors return exit 2 with no partial stdout. Paths must be repository-relative without parent traversal; external symlink targets are rejected. Each input is limited to 1 MiB and the pack to 4 MiB. These are context-size controls, not secret detection or a sandbox. Review private data before sharing. The standalone Python equivalent is python3 .factory/scripts/context_export.py with the same arguments.

## Quality command configuration

Configure .factory/checks.json with nonempty lists of argument arrays. The example at .factory/templates/checks.json is illustrative Python configuration, not a prescribed stack or sufficient project coverage. pip-audit requires a separate installation and relevant dependency environment. Adapt commands, test discovery, dependencies, and coverage to the actual project; a test command discovering zero tests is not evidence of coverage.

Commands execute sequentially from the repository root, inherit the environment, and stream output. A group stops at its first failure. No shell interpolation is performed. Put pipelines or complex workflows in an explicit project script. Never place secrets in argument arrays; use your normal secret manager or environment configuration.

Empty groups and malformed configurations return exit 2. Child command failures return exit 1. These helpers do not update task reports, review findings, or state automatically; agents must record evidence and assess actual coverage. Lifecycle helpers fail immediately on a held local lock. Assign separate worktrees; see [concurrent tasks and upgrade migration](../routines/concurrent-tasks.md).


## Findings register checks

`findings list` emits active summaries; `show ID` explicitly reads one record; `check` validates individual active/closed records and retained report pins. Version 2 creation uses `findings create --task ID --record JSON` with input fields from the finding template minus id. `next-id --task ID` suggests but does not reserve the next local sequence. Legacy v1 validation remains supported until [reviewed metadata migration](../routines/maintenance.md#upgrade-existing-projects). Closed records are never exported as context automatically.

`maintenance status|check` derives cadence from durable completed-task records; `audit-input --revision SHA --on DATE` produces explicit full-audit metadata. `metadata_upgrade.py --plan TASK --completions MANIFEST` previews legacy finding/history conversion, and `--apply PLAN` performs the exact reviewed plan with rollback. These helpers require task_store.py, findings.py, maintenance.py and git_snapshot.py together. Before/after task-state pruning run both findings/check and maintenance/check with --prune-task ID. See the canonical routines for formats, limitations and ownership.


## Pinned template updates

`./.factory/scripts/template-update init|plan|apply --project ROOT --source CHECKOUT --revision FULL_SHA` registers provenance, previews or integrates a reviewed upstream revision. Writes require `--task ID`, the matching task branch/selected execution state and a committed mapped baseline. `init` additionally requires a reviewed `--map JSON`; `apply` refuses conflicts unless individual reviewed resolutions are explicitly selected with `--accept-local PATH`. `--review-unmapped` acknowledges review of upstream changes outside the map. `fleet --source CHECKOUT --revision FULL_SHA --inventory JSON` plans registered adopters without writing them. See [the authoritative procedure](../routines/template-updates.md) for schemas, commands, protected data, exact exit codes, limitations and rollout evidence. Python 3.9+ and local Git supporting the documented offline/object flags are required; no network requests or upstream code execution occur in the helper.

## Cross-provider static review

`./.factory/scripts/review doctor|run|check|schema` (or `python3 .factory/scripts/review.py`) supports Codex and Claude CLI on macOS/Linux. See [configuration, exact examples, evidence and limits](../routines/cross-review.md). Review is disabled until a project commits an explicit provider/sharing policy. Doctor only probes version/help; the runner never installs software or logs in.

## Task identity/state upgrade

`python3 .factory/scripts/task_upgrade.py --plan OLD-UPGRADE-ID` prints a read-only migration plan; `--apply /outside/project/plan.json` applies the exact reviewed plan under the lifecycle lock. Optional `--project ROOT` supports invoking a trusted template helper before installation. It renames legacy archived tasks and the active upgrade task/branch, repairs mutable references and migrates execution state while retaining immutable pins and review identities. See [the required upgrade procedure and preservation limits](../routines/concurrent-tasks.md#upgrade-existing-task-ids-and-state). Keep task_store.py installed beside all dependent helpers.
