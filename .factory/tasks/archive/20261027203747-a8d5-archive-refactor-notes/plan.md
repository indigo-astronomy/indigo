# Development plan

Status: complete

## Approach and affected components
Mechanical archival with `git mv`, scripted removal of project-file entries, targeted text edits of rules, comments
and eight README files. Affected: `indigo_drivers/`, `indigo_linux_drivers/`, `indigo_mac_drivers/`,
`indigo_server/`, `indigo_test/`, `indigo_docs/`, `indigo_tools/driver_editor/AGENTS.md`, `tools/`, root
`AGENTS.md`, `indigo.xcodeproj/project.pbxproj`, `.factory/project.md`, `.factory/maintenance.json`.

## Baseline
- Branch `task/20261027203747-a8d5-archive-refactor-notes` from `master` `e0a2d5490` (equal to `origin/master`),
  clean tree; macOS Darwin 27.0.0 arm64.
- Existing failures: none known.

## Ordered steps
| Step | Change | Acceptance IDs | Completion check | Status |
| --- | --- | --- | --- | --- |
| 1 | Task ID, branch, record, audit deferral | - | task state valid | done |
| 2 | `git mv` of 122 files into `refactor/` | AC-001 | no tracked REFACTOR.md outside `.factory/`, 122 files | done |
| 3 | Remove vcxproj/filters/pbxproj entries; remove VS project refs from pbxproj | AC-002 | grep empty, `plutil -lint` OK | done |
| 4 | Redirect/remove rules in instructions and docs; Xcode VS rule | AC-003 | grep shows only intended mentions | done |
| 5 | Rewrite code/test/tool/protocol citations | AC-004 | grep, comment-only diff check, builds | done |
| 6 | Restructure eight README files | AC-005 | chapter list check, Testing diff empty | done |
| 7 | Checks, independent quality review, reports, archival, commit | AC-006, AC-007 | process check | done |
