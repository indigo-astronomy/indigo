# Task brief

- Task ID: 20261027203747-a8d5-archive-refactor-notes
- Owner: Peter Polakovic
- Created: 2026-10-27

## Problem and desired outcome
The INDIGO 3.0 refactoring process is being closed. Every tracked `REFACTOR.md` (122 files under `indigo_drivers/`,
`indigo_linux_drivers/`, `indigo_mac_drivers/` and `indigo_server/`) moves into this task's record as
`refactor/<containing folder name>.md` and is archived with the task. The rules that require creating and updating
`REFACTOR.md` are removed; obligations that only used `REFACTOR.md` as the place to record evidence are redirected
to the evidence of the Factory task performing the work. References in code, tests, tools and documentation point to
the archived files. Build and project files no longer list `REFACTOR.md`.

## Users and use cases
Maintainers and agents working on drivers and tests after the 3.0 refactoring; end users reading driver README files.

## Scope and non-goals
In scope, as agreed with the owner in chat on 2026-10-27:
1. Move all 122 `REFACTOR.md` files with `git mv`, named after their containing folder; the off-limits drivers
   `ccd_sbig` and `mount_asi` are included (only their `REFACTOR.md` moves, nothing else there changes);
   `indigo_server/resource/REFACTOR.md` becomes `indigo_server_resource.md`.
2. Remove their entries from `.vcxproj`/`.vcxproj.filters` files and `indigo.xcodeproj/project.pbxproj`.
3. Remove the `REFACTOR.md` rules from `AGENTS.md`, `indigo_drivers/AGENTS.md`, `indigo_test/AGENTS.md`,
   `indigo_test/DRIVER_TESTING_RULES.md`, `indigo_tools/driver_editor/AGENTS.md`, `.factory/project.md` and the
   developer docs; redirect recording obligations to task evidence; keep the MIGRATION_STATUS.md rule itself.
4. Rewrite comments in code, tests and tools that cite `REFACTOR.md` to cite the archived file.
5. The eight driver README files that mention `REFACTOR.md` (aux_mgbox, aux_simulator, ccd_atik, ccd_playerone,
   focuser_efa, focuser_ioptron, focuser_lacerta, focuser_prodigy) keep only the fixed user-facing chapters
   `## Supported devices`, `## Supported platforms`, `## License`, `## Use`, `## Status`, `## Notes` and
   `## Testing`; refactoring notes and other chapters are removed (owner instruction replacing recommendation 3).

Non-goals: behavior changes, driver version bumps, the line in the root changelog rules excluding `REFACTOR.md` from
changelog entries (commits in the next release range still touch these files), README files that do not mention
`REFACTOR.md`, push.

## Constraints and references
Root `AGENTS.md`, `indigo_drivers/AGENTS.md`, `indigo_test/AGENTS.md`, `.factory/routines/task-archive.md`. Xcode must
not reference `.factory/`. The full repository audit is due; the owner deferred it for this task (recorded in
`.factory/maintenance.json`, `refactor_notes_archive_deferral`).

## Initial questions
Resolved in chat: recommendations 1, 2, 4, 5 and 6 accepted; README handling replaced as described in scope item 5.
