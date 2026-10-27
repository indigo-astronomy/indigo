# Specification

Status: accepted

## Problem and outcome
The INDIGO 3.0 refactoring process is closed by the owner (chat, 2026-10-27). Its driver-local notes are archived in
this task and the rules that required them are withdrawn. See `brief.md` for the agreed scope.

## Acceptance criteria
- AC-001: no tracked `REFACTOR.md` remains outside `.factory/`; all 122 former files exist in this task's `refactor/`
  directory, each named after its containing folder (`indigo_server/resource` -> `indigo_server_resource.md`), moved
  with history-preserving `git mv` and unchanged in content.
- AC-002: no `.vcxproj`, `.vcxproj.filters` or `project.pbxproj` entry references `REFACTOR.md`; `project.pbxproj`
  references no Visual Studio project file and nothing under `.factory/`, and passes `plutil -lint`.
- AC-003: no instruction or developer document requires creating, updating or recording into `REFACTOR.md`; each
  former recording obligation names the Factory task record instead; root `AGENTS.md` states that Visual Studio
  project files do not belong in the Xcode project; the `MIGRATION_STATUS.md` rule and the changelog exclusion of
  `REFACTOR.md` remain.
- AC-004: every remaining citation of a former `REFACTOR.md` in code, tests, tools and protocol documents names an
  existing archived file under `.factory/tasks/archive/20261027203747-a8d5-archive-refactor-notes/refactor/`;
  code changes are comments only, apart from the OmniSim "NOT RUN" message text; no driver version changes.
- AC-005: the eight README files (aux_mgbox, aux_simulator, ccd_atik, ccd_playerone, focuser_efa, focuser_ioptron,
  focuser_lacerta, focuser_prodigy) contain exactly the chapters Supported devices, Supported platforms, License,
  Use, Status, Notes and Testing in that order; user-relevant information (URLs, ports, baud rates, property
  renames, operating notes) is in `## Notes`; refactoring/developer notes are gone; `## Testing` is unchanged.
- AC-006: the owner's Xcode re-save of `project.pbxproj` is included unchanged (owner request).
- AC-007: Factory check, security and test helpers pass; the changed driver and test still build.

## Out of scope
Behavior changes, README files that never mentioned `REFACTOR.md`, other Xcode metadata, push.
