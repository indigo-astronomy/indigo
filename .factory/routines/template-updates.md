# Updating adopted projects from AI Factory

## Ownership and provenance

The template publishes policy and helpers; each adopting project owns its integration. Updates are pulled into a project task branch, reviewed and tested there, then delivered under that project's Git policy. A template commit does not grant access to downstream repositories or permission to push/merge them. No scheduler, webhook, background agent or remote deployment is installed.

The versioned `.factory/template-lock.json` records `version: 1`, the approved credential-free `repository` URL, a full `revision` commit SHA and `files`, an explicit object mapping upstream paths to project-relative destinations. The revision means the baseline considered during the last reviewed integration, including explicitly retained local customizations; it does not claim every file is identical to upstream. The template repository itself has no lock pointing to itself.

Use [the mapping example](../templates/template-map.json) as an inventory, not an install manifest. Adapt destinations for relocated helpers and host instructions. Include each dependency required by the selected policy; review omitted paths deliberately. Source and destination paths must be unique, nonoverlapping, plain relative paths. The helper accepts at most 256 UTF-8 text files of at most 1 MiB each, including executable text scripts. Symlinks, submodules, binary files, traversal and Git metadata cannot be transferred. Other file types require a separate manual migration.

Project-owned `project.md`, `checks.json`, `process-policy.json`, `review-policy.json`, `review.local.json`, `state.json`, `maintenance.json`, tasks, findings/counter/archive, legacy telemetry data, local adopter inventory and the lock itself cannot be mapped on either side. Legacy `state.yaml` and `maintenance.yaml` remain protected too. Upstream changes to their schemas require a reviewed manual migration preserving actual project data. Other project-specific files, including approved model/budget policies, should stay outside the map when the owner wants full local ownership. Mapping shared instructions allows three-way integration; it never overrides existing project authority.

## Register an adoption

New migrations use the same immutable TEMPLATE_COMMIT for installation and provenance. After reviewed file-by-file integration, create the adoption task and initialize its lock using the reviewed helper:

```sh
./.factory/scripts/template-update init --project "$PROJECT_ROOT" --source "$TEMPLATE_CHECKOUT" \
  --revision "$TEMPLATE_COMMIT" --map /path/to/reviewed-map.json --task 001-adopt-ai-factory
```

Variables denote separate absolute checkout paths and a full pinned SHA, not moving refs. Run from the matching `task/001-adopt-ai-factory` branch with state.json selecting that task. Initialization writes only the lock, refuses to replace an existing lock, and preserves customized or missing target files. Commit the map decision, lock and installed baseline with the migration report before subsequent upgrades. Ensure the local inventory filename is ignored in the adopting project before using it. New empty projects exported from the template register their exact exported SHA through the same process after Git/task setup; it is not inferred from current upstream main.

For older adoptions, identify the exact original template commit from their adoption report/Git evidence, review the source-to-target inventory and initialize on a dedicated task branch. If the baseline cannot be established, perform a manual migration/inventory first; do not guess a baseline or label the current template as already integrated. A missing updater may be bootstrapped from a separately reviewed pinned release, or invoked from that trusted checkout after reviewing its code. The source passed to the helper is otherwise treated as data.

## Discover and plan an update

At a project session start, inspect the lock and any recorded pending update in the task handoff. When network access is authorized and a refresh is appropriate, fetch the approved upstream's main in a separate checkout once, resolve it to a full SHA and keep that pin for the entire upgrade. Offline work keeps its existing pin and records discovery as unavailable. No network request is made by the helper. Git must support `--no-replace-objects`, `--no-lazy-fetch` and `--no-optional-locks`; unsupported Git fails closed. Missing partial-clone objects are not fetched implicitly; obtain approved objects explicitly before replanning. Verify the source origin matches the lock; URL matching and Git object identity are not release signing or an authenticity guarantee.

```sh
./.factory/scripts/template-update plan --project "$PROJECT_ROOT" --source "$TEMPLATE_CHECKOUT" \
  --revision "$TEMPLATE_COMMIT"
```

Planning ignores Git replacement refs and reads old/new blobs from Git objects, so dirty files or a different HEAD in the source checkout do not change the selected content. Both commits must be available, and the new commit must descend from the baseline. A source checkout and target must be separate non-nested Git roots for writes. The report lists mapped actions (`unchanged`, `add`, `update`, `delete`, `conflict`) and **all upstream changed paths outside the map**, including additions and project-owned schemas. Inspect their diffs and dependencies before proceeding. No file contents or archived findings are included in the report. `plan` changes nothing; exit 0 means no mapped conflict, not completed integration.

When new shared files are needed, add reviewed mappings to the project lock before applying, retaining the old revision. The old blob may be absent for an added file. For a deliberate relocation, first move the actual local file and update its destination mapping; commit that reviewed baseline. Do not edit `revision` to bypass conflicts. Excluding an obsolete mapped path means explicitly accepting local ownership, not automatically deleting the local file.

The updater recognizes the historical upstream helper move from `scripts/NAME` to `.factory/scripts/NAME`: when the old source exists at the pin, disappears in the new revision and its new regular blob exists, the plan reports `renamed_source` and merges against the old blob. A successful apply changes those source keys and the revision together, retaining destinations. This is a bounded compatibility rule, not general Git rename detection or automatic local relocation. Conflicts leave files, keys and pin unchanged; an already-mapped replacement is rejected. Without a replacement, ordinary deletion/conflict rules apply. See the migration below before updating an existing adopter.

## Integrate in the adopting project

1. Create/resume a dedicated upgrade task under the project's Git/workflow policy, with brief, handoff and active state. The helper's write guard supports the bundled `task/<task-id>` convention; projects with another convention use a reviewed manual integration or adapt the guard deliberately. Commit the mapped files and lock first. Task records may remain in progress; unrelated paths are not written by the updater.
2. Review the plan, upstream diff, instruction changes, compatibility and unmapped dependencies. The helper performs textual merging, not semantic or security approval. Executable permissions are merged along with content; upstream scripts are never executed by the helper. Existing local-only changes are retained. Upstream deletion of a locally modified file is a conflict.
3. Apply the exact pin:

```sh
./.factory/scripts/template-update apply --project "$PROJECT_ROOT" --source "$TEMPLATE_CHECKOUT" \
  --revision "$TEMPLATE_COMMIT" --task 002-update-ai-factory --review-unmapped
```

Use `--review-unmapped` only after inspecting and resolving every reported omission; omit it when no unmapped paths changed. It acknowledges deliberate exclusions/manual migrations, not automatic installation of missing dependencies. The write guard requires the matching task branch/active pointer and committed mapped files/lock. Any mapped conflict prevents **all** writes and keeps the old pin; exit 1 includes the conflict report. Invalid input, wrong source, dirty mapped paths or unavailable Git objects exit 2.

4. Resolve conflicts manually using the old commit, new commit and current file. Review the complete resulting local file (or deliberate deletion), commit that baseline, then rerun at the same pin with `--accept-local path/to/file` for each reviewed resolution. This explicitly retains the local result for those paths while integrating the rest; no content-marker heuristic constitutes approval. Use the same option with `plan` to preview the result. Record each decision and accepted omission in the task report.
5. Verify the actual final diff, scoped instruction loading, helper compatibility, findings integrity and project-specific regressions; complete quality/security gates. Commit evidence and the advanced lock. Applying a pin is **not** task completion or permission to publish. Keep unresolved findings in the project register; ordinary adopted projects retain task evidence.

Each file is replaced atomically and the lock is written last. Ordinary write errors trigger best-effort restoration of original file contents/modes. The entire operation is not crash-atomic; interruption, storage failure or another writer can require recovery. Keep one writer and a committed baseline. After interruption inspect Git status and restore only updater changes through reviewed patches from that baseline, preserve unrelated task work, then replan at the same pin. Do not use destructive reset/clean. Reapplying an already committed same pin makes no changes.

## Distribute a template release to registered projects

After a template change passes its gates, the template coordinator records the release's exact tested commit in delivery evidence. Use the owner-approved published revision once available; local testing may use an explicitly identified unpublished candidate. Never distribute a moving branch name. Do not automatically publish a release merely because a task is complete.

Maintain an owner-approved **local** `.factory/adopters.local.json` inventory, excluded from Git and routine context. Entries contain a unique lowercase `name` and absolute checkout `path`; no credentials or tokens. Start from [the synthetic inventory example](../templates/adopters.json).

Registration is opt-in; the README migration prompt explicitly opts the target project in, while other registrations need applicable owner authorization. Keep the inventory in a persistent local registry checkout, independently of a disposable pinned source checkout. Preserve that checkout's branch and tracked/local work; validate its origin. Ensure Git ignores the inventory before writing (an exact local exclude can avoid tracked changes). Update valid JSON atomically while detecting concurrent changes, preserve other entries and reuse the canonical project's existing entry; choose a unique alias for a new path. Resolve invalid/duplicate/conflicting entries rather than overwrite them. Verify the target entry with `fleet` at the migration pin; distinguish other unavailable projects from target registration failure. GitHub cannot discover all copies of a template. Owners can keep the inventory elsewhere and pass its path explicitly.

```sh
./.factory/scripts/template-update fleet --source "$TEMPLATE_CHECKOUT" --revision "$TEMPLATE_COMMIT" \
  --inventory .factory/adopters.local.json
```

The dispatcher plans the same pin for every registered adopter using each project's own lock and path map. It writes no adopter files, fetches nothing and never runs their scripts. Its JSON report marks projects `current`, `pending`, `conflict` or `unavailable`; exit 1 means at least one conflict/unavailable project, exit 2 invalid invocation/inventory. Continue processing other entries when one is offline or unregistered. Record a release checklist in the template task handoff (use project aliases rather than private paths): previous pin, requested pin, responsible owner, status/blocker and next action. On task cleanup retain that committed evidence; unresolved distribution follow-up must have an active finding/task reference before pruning.

For each pending project, the authorized coordinator follows the project-local integration steps above. Rerun `fleet` after integrations; `current` only confirms the lock pin, so also retain each project's verified task/delivery evidence. Offline or conflicted adopters remain pending, never silently counted as updated. The template closure review must either record no registered projects, verified per-project outcomes, or accountable unresolved follow-up. Actual remote publication and cleanup remain separate authorized actions.

## Move existing Factory helpers into .factory/scripts

New adoptions install helpers in `.factory/scripts/`; application-owned `scripts/` stays separate. There are no old-path wrappers or symlinks. For an adopter pinned before this upstream move:

1. Prepare the project's upgrade branch and task using its **currently working helper path** before moving anything. Preserve the existing active task when applicable. Inspect the lock and the old/new pinned source inventories. Review `.factory/scripts/template_update.py` at the new immutable release and invoke that trusted copy explicitly for this migration; the old installed updater does not recognize source relocation. Obtain and review its complete code before execution, under existing host permissions. Do not execute unreviewed source merely because it appears in a plan.
2. Inventory each installed Factory file using the existing lock and adoption evidence, including its wrappers, all adjacent Python modules and README. Move only those files to `.factory/scripts/` with `git mv OLD_DEST NEW_DEST` after checking that each destination is unused and real. Do not move the entire application `scripts/` directory or overwrite `.factory/scripts/`. Preserve local edits and executable bits. For example, a default installation uses `git mv scripts/new-task .factory/scripts/new-task`; create the destination directory first. Files without provenance need reviewed registration/inventory rather than guessed ownership. Leave archived reports and hashed review packets unchanged.
3. In the existing lock, change only each relocated **destination value**, keeping old source keys such as `"scripts/new_task.py"` and the original `revision`. For example, the transitional entry is `"scripts/new_task.py": ".factory/scripts/new_task.py"`. Retain other mappings and instance data. Commit this recoverable relocation baseline on the upgrade task branch before `apply`; the updater requires clean mapped paths and lock. This meaningful migration checkpoint is an explicit exception to the usual single-result-commit default. Record its reason in the task handoff. Moved root-aware Python helpers still contain their old depth calculation until merged, so use the reviewed external updater during this interval.
4. Run the reviewed new updater from its trusted checkout, keeping the new release pin fixed throughout:

   ```sh
   python3 "$TEMPLATE_CHECKOUT/.factory/scripts/template_update.py" plan \
     --project "$PROJECT_ROOT" --source "$TEMPLATE_CHECKOUT" --revision "$TEMPLATE_COMMIT"
   python3 "$TEMPLATE_CHECKOUT/.factory/scripts/template_update.py" apply \
     --project "$PROJECT_ROOT" --source "$TEMPLATE_CHECKOUT" --revision "$TEMPLATE_COMMIT" \
     --task TASK-ID --review-unmapped
   ```

   Replace TASK-ID with the actual active upgrade task. Use `--review-unmapped` only after resolving every reported omission. The plan's `source` is the old merge-base key, `renamed_source` is its new upstream path, and `target` is the local destination. The updater merges old upstream content, relocated local content and new upstream content. It advances source keys and the pin together only after all mapped conflicts are resolved. Do not preemptively replace source keys or initialize a new lock: that would discard the old merge base. An unchanged local helper whose upstream file moved must be updated, not deleted/reinstalled as an unrelated addition.
5. Resolve conflicts using the normal reviewed local resolution procedure. `--accept-local PATH` preserves the chosen local file but still adopts the new source key/pin, so first reconcile required root/path changes in that local file and commit the resolution. Standard root-aware helpers now use `Path(__file__).resolve().parents[2]`. The updater never changes destination values automatically. Intentionally retaining an alternative location requires manually adapting root resolution and live command references for that depth, or retaining the compatible local calculation through reviewed conflict resolution. Keep the Python support modules together.
6. Update project-owned `.factory/checks.json`, `.factory/process-policy.json`, `.factory/review-policy.json` share paths, CI, package commands, host adapters, project.md and live task instructions where they invoke or classify Factory helper paths. These instance configurations are protected and are never automatically replaced by a template update. Custom process risk rules must cover `.factory/scripts/` sensitive helpers while retaining legacy paths needed for history. Preserve application-native script commands. Regenerate the decision index if present. Test the installed wrappers, lifecycle/context/findings/process/review regressions and application checks from the project root and from an unrelated cwd; never contact a provider just to test a path move.
7. Verify the final lock maps new `.factory/scripts/NAME` source keys to the intended real files, and no old Factory copies remain. Remove an empty old directory only after confirming it contains no native/untracked files. Retain upgrade evidence and complete delivery under the project Git policy. Rollback uses the committed relocation baseline and old pin plus reviewed reverse moves/map edits; preserve subsequent local edits, findings, state and maintenance. No reset, wholesale directory deletion, provenance repinning shortcut or downstream publication is implied.

## Migrating workflow and maintenance data to JSON

Factory-owned structured files use JSON; Markdown remains the documentation format. Native manifests belonging to external tools keep their required formats. Workflow state stays at `.factory/state.json`, outside task directories.

Existing adopters must migrate deliberately on their task branch before running upgraded helpers:

1. Preserve the current branch, active task/handoff and a committed baseline. Inspect the old `.factory/state.yaml` and `.factory/maintenance.yaml`; do not replace them with template defaults.
2. Convert each YAML mapping into the corresponding JSON object at `state.json` and `maintenance.json`, retaining every key, value, null, boolean, number and nested checkpoint mapping, including project-specific fields. Resolve YAML-specific types explicitly; transfer useful comments to the project documentation because JSON has no comments.
3. Validate JSON syntax and compare decoded values against the old mappings. Confirm the same active task, role, phase/status, blocker and maintenance counters/review pointers. Resolve any existing JSON destination rather than overwriting it. Review all local consumers, instruction references and context-export paths.
4. Remove the two old YAML files only after comparison. Commit the conversion together with updated consumers/references and run project regressions, task lifecycle checks, findings and process checks. Keep historical reports and findings unchanged.

The helpers read JSON only and reject malformed, duplicate-key or non-object state before authorizing writes. There is no fallback that initializes a missing JSON file from export defaults and no automatic downstream conversion. Legacy instance paths remain excluded from template mappings so an upgrade cannot overwrite their data. Existing adopters whose lock maps the removed `.rgignore` must review that obsolete mapping and retire it deliberately; preserve any project-owned search exclusions.

## Retired usage reporting

The template no longer ships `scripts/usage`, `scripts/usage_report.py`, `.factory/usage.md`, the usage-event template or the telemetry README. They summarized manually supplied events without a collector. Automatic attribution to factory tasks and roles was not implemented; do not present raw host session totals as project task metrics.

When upgrading an adopter, review removed mappings for those paths against the existing provenance pin and any project-owned customization. Remove only the retired template-owned helpers/documentation; keep private local ledgers and summaries, their Git ignore rules, and the protected `.factory/telemetry` path. The upgrade helper still protects legacy instance data. No collector, host hook, background service or template-maintainer usage capture is installed.
