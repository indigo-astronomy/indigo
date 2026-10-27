# Portable decision and process checks

## Decision navigation

The canonical long-lived decisions remain ADRs in `indigo_docs/decisions/`, using `template.md` metadata. Name records
`ADR-NNN-short-title.md`; the heading ID must match, IDs must be unique, and status is `proposed`, `accepted` or
`superseded`. Date is an ISO calendar date; owner and related task are explicit. Keep operational notes in task
handoffs; promote durable choices to ADRs with a link to their source evidence. No approval is inferred from metadata.

Run `python3 .factory/scripts/decision_index.py` after ADR changes. It generates `indigo_docs/decisions/README.md` with links,
status, date, owner and related task, in numeric ID order. `--check` writes nothing and exits 1 for stale/missing output.
Only immediate ADR files are read; templates and task/findings archives are never mined. Invalid metadata,
symlink input/output or inputs over 1 MiB return 2 before writing. The index is navigation, never a second authority.

## Committed process evidence

Run from a checkout containing the helpers, using Python 3.9+ and Git with the offline flags documented in the
[update routine](template-updates.md):

```sh
python3 .factory/scripts/process_check.py --base BASE_COMMIT --head HEAD
```

`./.factory/scripts/process-check` is the POSIX convenience wrapper. Other CI systems invoke the Python command directly.
The check reads immutable local commits, not dirty files, executes no recorded commands or candidate code, and
fetches nothing. The base must be an ancestor of head; for an advanced PR target, pass `git merge-base TARGET HEAD`.
Full local history is required. Invalid refs, malformed inputs, unsupported Git or a shallow repository return 2.

Changed task IDs are discovered from `.factory/tasks/` changes across the range, including evidence committed then
pruned. Optional repeated `--task ID` restricts selection to changed tasks. Every changed path must be in at least
one selected task's scope. A task's own evidence paths are covered automatically. Each task needs populated brief,
specification, plan, test plan, handoff and final verification.md, security.md and quality-review.md reports. Each final report has
one `Verdict: pass` line (an optional `- ` prefix is accepted); unresolved placeholders fail. Partial work remains
on its branch and the delivery check stays red until its gates are complete. A recurring audit's repository-review.md is separate and cannot replace quality-review.md. For old task reports and role-policy mappings, follow the [quality-review upgrade steps](quality-review.md#upgrade-existing-projects).

New tasks use version 2 `process.json`:

```json
{
  "version": 2,
  "task": "20261022143012-a81f-example",
  "reviewed_content": {
    "app/example.py": ["100644", "blob", "FULL_GIT_OBJECT_ID"],
    "app/removed.py": null
  },
  "scope": ["app/*", "tests/*", "docs/*"],
  "checks": [
    {"command": ["python3", "-m", "unittest"], "result": "pass", "evidence": "verification.md"}
  ],
  "risk_reviews": {"authentication": "security.md"},
  "reviews": []
}
```

After review and archival, stage assigned source files and print their changed identities:

```sh
python3 .factory/scripts/process_check.py --base BASE_COMMIT --task 20261022143012-a81f-example --record-content
```

Place this output object in `reviewed_content`, then stage process.json and the final reports. The recorder reads
the index and the selected live/archive manifest scope, not dirty source or untracked files; it writes nothing,
stages nothing and executes no recorded commands. It rejects unresolved index stages and symlink manifest paths.
Inspect the staged diff and resolve unstaged source changes before committing. Capturing hashes is not review.
The normal `--base ... --head ...` gate continues to read immutable committed objects only.

Each changed source/config/doc path must have a covering task's exact Git mode, object type and full object ID in
the final candidate. Deletions require an explicit null entry; absence is not deletion evidence. SHA-1 and SHA-256
repositories use their native object IDs. Invalid paths, identities, scopes, missing reports and stale content fail.
Task evidence and state/maintenance bookkeeping are excluded from source identity, avoiding circular hashes. Scope
coverage still applies to changed files. Scope matching uses case-sensitive Python `fnmatchcase` over full relative
paths (`*` can span `/`); keep scopes narrow. Records describe review claims, not authenticated approvals.

Record actual commands/results, working directory, environment and limitations in the reports. Argument arrays
are records only; the helper never runs them. Findings validation preserves schema, archive/counter and retained-pin
ancestry checks; unresolved blocking findings fail. The decision index must be current. Exit 0 means structural
success, exit 1 process violations, exit 2 unavailable/invalid input. Empty configured check groups remain unconfigured.

Complete and archive task execution before the single final commit, with integration/publication honestly pending.
Both ordinary projects and the template keep evidence at `.factory/tasks/archive/TASK-ID/`. The template excludes
its archive from new-project exports under [template-maintenance.md](template-maintenance.md). Verify the final
committed candidate and report its SHA/post-commit gate outcomes externally; do not commit a hash referring to itself.

### Compatibility and exceptional checkpoints

Version 1 evidence with `reviewed_revision` remains accepted and compares source against the retained ancestor
commit exactly as before. Do not rewrite published evidence. Legacy pruned-template ranges still resolve the most
recent manifest-bearing snapshot in `BASE..HEAD` (at most 200 task-manifest commits); required evidence ancestors
must remain reachable. New tasks need no intermediate source/evidence commits or pruning.

Opt-in cross-provider packets remain commit-bound and require version 1 process evidence. A project requiring
those roles records a checkpoint exception to the single-commit default; keep the source commit and review evidence.
Version 2 cannot cite such packets or bypass baseline-required roles. See [cross-review.md](cross-review.md).

## Diff risk and autonomy

For a committed candidate, inspect reasons without requiring completed task evidence:

```sh
python3 .factory/scripts/process_check.py --base BASE_COMMIT --risk-only
python3 .factory/scripts/process_check.py --show-policy
```

`--risk-only` reports signals and does not certify the process. Built-in categories cover authentication/permissions,
data model/migrations, process policy and CI/deployment paths. Dependency detection compares package names across
the four standard `package.json` dependency groups and normalized names in requirements text files; version-only
updates of those entries do not trigger `dependencies`. Changed directives/URLs are conservatively included.
Other listed manifests/lockfiles trigger dependency review on any change; no incomplete language parser claims
otherwise. Reasons contain category, changed path and required report; package names/URLs are not printed.
Deleted paths and both sides of renames are checked. Task evidence paths do not classify product risk.

To customize, save the `--show-policy` JSON as project-owned `.factory/process-policy.json` and review its rules.
Each rule has a unique `id`, a `paths` list of case-insensitive full-path globs and `report` (`security.md` or
`verification.md`). `dependencies` is reserved; `dependency_manifests` lists additional globs (unknown formats use
conservative review). Rules are evaluated from the supplied baseline commit; absent configuration uses built-in
defaults. A candidate policy is validated but becomes effective only for later ranges. Preserve this file during
template upgrades; it cannot be mapped by the updater.

Each triggered category needs a covering task's `risk_reviews` entry pointing to the required passing report. The
reviewer explains actual risks, findings and checks there. These are specialist evidence requirements, not new
human gates: ordinary work stays autonomous, and existing owner authorization remains valid. Existing permissions
for merging, deploying, new charges and accepting risk still apply.

Signals depend on paths and configured manifests; renamed concepts, external services, dynamic dependencies and
semantic security changes can escape detection. Conservative patterns can flag documentation. Reviewers remain
responsible for semantic risk and should tailor policy to the discovered layout.

## Local verification and optional CI

Local commands are the reference workflow. Run the project's configured checks and relevant regressions, regenerate
or check the decision index, and run the committed-range process gate before delivery. These helpers need only local
files, Python and Git; no GitHub account, hosted runner, token, browser login or network service is required.

A project may optionally call the same commands from any CI platform using its own configuration and existing owner
authorization. Supply complete local history and explicit base/head commits. This template installs no hosted CI
workflow, and completion never depends on a particular service's availability or status page. A missing or failing
local required check remains a failure; removing hosted integration does not waive verification or review gates.

The checker and reports do not authenticate approvals. A writer who can replace the checker can change its behavior;
baseline-owned data rules prevent self-downgrading through configuration, not malicious replacement of the code.
Projects that need additional enforcement choose their own trusted execution, policy ownership and repository
protections. Local scripts, manifests and passing documents are not a sandbox or an authenticated approval system.

## Optional cross-provider evidence

The optional `reviews` field and baseline-owned required reviewer roles follow [cross-review.md](cross-review.md#committed-process-evidence). The gate reconstructs committed packets, rejects stale/failed/inconclusive evidence and preserves the existing source-identity checks. It never invokes a provider or authenticates a review.
